#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/time.h>

#include "rknn_api.h"
#include "tcp_comm.h"
#define SERVER_PORT	1808

static rknn_context rknn_ctx;
static rknn_input_output_num in_out_num;
static rknn_tensor_attr tensor_attr[8];
static class tcp_comm tcp_server;

//static const char *MODEL_PATH = "../models/ssd/mobilenet_ssd.rknn";

static unsigned long get_time(void)
{
	struct timeval ts;
	gettimeofday(&ts, NULL);
	return (ts.tv_sec * 1000 + ts.tv_usec / 1000);
}

int load_model(const char *model_path, uint32_t flag)
{
	int ret = 0;
	FILE *fp = NULL;
	int model_len = 0;
	unsigned char *model = NULL;

	/* Load model to bufffer */
	printf("loading model...\n");
	fp = fopen(model_path, "rb");
	if (fp == NULL) {
		printf("fopen %s fail!\n", model_path);
		ret = RKNN_ERR_MODEL_INVALID;
		goto exit;
	}

	fseek(fp, 0, SEEK_END);
	model_len = ftell(fp);
	model = (unsigned char *)malloc(model_len);
	if (!model) {
		printf("Malloc %d buffer fail!\n", model_len);
		ret = RKNN_ERR_MALLOC_FAIL;
		goto exit;
	}

	fseek(fp, 0, SEEK_SET);
	if (model_len != fread(model, 1, model_len, fp)) {
		printf("fread %s fail!\n", model_path);
		goto exit;
	}

	/* load model to npu */
	ret = rknn_init(&rknn_ctx, model, model_len, flag);
	if (ret < 0) {
		printf("rknn_init fail! ret=%d\n", ret);
		goto exit;
	}

	ret = rknn_query(rknn_ctx, RKNN_QUERY_IN_OUT_NUM, &in_out_num,
			 sizeof(in_out_num));
	if (ret)
		goto exit;

	printf("input num = %u, output = num = %u\n",
	       in_out_num.n_input, in_out_num.n_output);

	for (int i = 0; i < in_out_num.n_output; i++) {
		tensor_attr[i].index = i;
		ret = rknn_query(rknn_ctx, RKNN_QUERY_OUTPUT_ATTR,
				 &tensor_attr[i], sizeof(rknn_tensor_attr));
		if (ret < 0) {
			goto exit;
		}
		printf("tensor index %d is %s, type = %02x, qnt_type = %02x\n",
		       i, tensor_attr[i].name, tensor_attr[i].type, tensor_attr[i].qnt_type);
	}

exit:
	if (fp)
		fclose(fp);
	if (model)
		free(model);

	return ret;
}

int inference_time_tmp =0;
float* inference_ret_debug;
float* inference_ret_debug_pre=NULL;
int inference(void *in_data, int in_len)
{
	int ret = 0;
	rknn_input input;
	rknn_output outputs[in_out_num.n_output];
        long start_time = get_time();
	memset(&input, 0x00, sizeof(input));
	input.index = 0;
	input.buf   = in_data;
	input.size  = in_len; // 
	input.pass_through = false;
	input.type  = RKNN_TENSOR_UINT8;
	//input.type  = RKNN_TENSOR_FLOAT32;
	input.fmt   = RKNN_TENSOR_NHWC;

	ret = rknn_inputs_set(rknn_ctx, 1, &input);
	if(ret < 0) {
		printf("rknn_input_set fail! ret=%d\n", ret);
		return ret;
	}

	ret = rknn_run(rknn_ctx, nullptr);
	if(ret < 0) {
		printf("rknn_run fail! ret=%d\n", ret);
		return ret;
	}
        
	memset(outputs, 0, sizeof(outputs));
	for (int i = 0; i < in_out_num.n_output; i++) {
		outputs[i].want_float = 1;
	}
	ret = rknn_outputs_get(rknn_ctx, in_out_num.n_output, outputs, NULL);
	if(ret < 0) {
		printf("rknn_outputs_get fail! ret=%d\n", ret);
		return ret;
	}
	bool check_fail = false;
	bool check_fail_pre = false;
        if (inference_time_tmp == 0) {
            inference_time_tmp++;
            inference_ret_debug = (float*)malloc(outputs[0].size);
            memcpy(inference_ret_debug, outputs[0].buf, outputs[0].size);
            inference_ret_debug_pre = (float*)malloc(outputs[0].size);
            memcpy(inference_ret_debug_pre, outputs[0].buf, outputs[0].size);
        } else {
	    
            for (int i = 0; i < outputs[0].size/4; ++i) { // 4 is the size of float
                if (((float*)inference_ret_debug)[i] != ((float*)outputs[0].buf)[i]) {
                    //printf("check error! %f, %f \n", ((float*)inference_ret_debug)[i] , ((float*)outputs[0].buf)[i]);
		    check_fail = true;
                } else {
                    //printf("check OK! %f, %f \n", ((float*)inference_ret_debug)[i] , ((float*)outputs[0].buf)[i]);
                }
            }
	    // check with the pre results
	    for (int i = 0; i < outputs[0].size/4; ++i) {
                if (((float*)inference_ret_debug_pre)[i] != ((float*)outputs[0].buf)[i]) {
                    check_fail_pre = true;
                } else {
                }
            }
            memcpy(inference_ret_debug_pre, outputs[0].buf, outputs[0].size);
        }
	if (check_fail) {
            printf("CHECK FAIL [first]!!  ");
	}
	if (check_fail_pre) {
            printf("CHECK FAIL [pre]!!    ");
	}
        printf("forward takes time: %d ms\n" ,get_time() - start_time);
	//fprintf(stderr,"in_out_num.n_output = %d\n", in_out_num.n_output);
//	for (int i = 0; i < in_out_num.n_output; i++) {
//		fprintf(stderr,"outputs[i].size = %d\n", outputs[i].size);
//		//cv::Mat m = cv::Mat(256,256,CV_32F, outputs[i].buf);
//		//cv::imwrite("/dev/shm/a.jpg");
//		ret = tcp_server.send_data(outputs[i].buf, outputs[i].size);
//		if (ret != outputs[i].size) {
//			printf("send len error: %d; expect len: %d\n", ret, outputs[i].size);
//			return -1;
//		}
//	}

	rknn_outputs_release(rknn_ctx, in_out_num.n_output, outputs);

	return 0;
}

int main(int argc,char**argv)
{
	int ret = 0;
	void *input_buf = NULL;
	int in_len = 0;
	int recv_len = 0;

//	ret = tcp_server.tcps_accept();
//	if (ret) {
//		printf("accept error\n");
//		exit(1);
//	}
        if (argc < 2){
	    printf("input args error!\n");
            exit(1);
	} 
	//ret = load_model(MODEL_PATH, 0);
	ret = load_model(argv[1], 0);
	if (ret) {
		printf("load model error\n");
		goto exit;
	}

//	ret = tcp_server.send_data(&in_out_num, sizeof(in_out_num));
//	if (ret != sizeof(in_out_num)) {
//		printf("send len error: %d; expect len: %d\n", ret, sizeof(in_out_num));
//		goto exit;
//	}
//
//	ret = tcp_server.send_data(&tensor_attr[0], sizeof(rknn_tensor_attr) * in_out_num.n_output);
//	if (ret != sizeof(rknn_tensor_attr) * in_out_num.n_output) {
//		printf("send len error: %d; expect len: %d\n", ret,
//			sizeof(rknn_tensor_attr) * in_out_num.n_output);
//		goto exit;
//	}
//
	input_buf = malloc(4*1024*1024);
	if (!input_buf) {
		printf("alloc memory error\n");
		goto exit;
	}
        // init inputs as constants
        for (int i =0; i < 1024*1024; ++i) {
            ((float*)input_buf)[i] = i;
        }
	while (1) {
		recv_len = 0;
                //printf("start recieve size data ... \n");
                //fflush(stdout);
		//ret = tcp_server.recv_data(&in_len, sizeof(in_len));
		//if (ret != sizeof(in_len)) {
		//	printf("recv len error: %d; expect len: %d\n", ret, sizeof(in_len));
		//	goto exit;
		//}
                //printf("start recieve input data... \n");
                //fflush(stdout);
		//ret = tcp_server.recv_data(input_buf, in_len);
		//f (ret != in_len) {
		//	printf("recv len error: %d; expect len: %d\n", ret, in_len);
		//	goto exit;
		//}
                //printf("start inference ... \n");
                //fflush(stdout);
                
		//ret = inference(input_buf, 4*1024*1024); // in_len is not important??
		ret = inference(input_buf, in_len); // in_len is not important??
		//if (ret){
		//	goto exit;
                //}
        }
        if (inference_ret_debug) {
            free(inference_ret_debug);
	}
	if (inference_ret_debug_pre) {
            free(inference_ret_debug_pre);
	}
exit:
	if (input_buf)
		free(input_buf);
	if (rknn_ctx >= 0)
		rknn_destroy(rknn_ctx);

	tcp_server.destroy();

	return 0;
}

