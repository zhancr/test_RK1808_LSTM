# 验证多模型并行问题
 1. 代码是在RK提供的官方server代码上修改的，启动后将不停地循环做推理
 2. 运行方法: `mkdir build`, 在buld文件夹中执行`cmake ..` 和 `make` 后，执行 `./1808_debug ../models/model_name_xxx`
 3. librknn_runtime version 1.5.1, driver version 10126
 4. models文件夹内提供了用于测试的模型, 模型文件说明：
 
|  模型  |  路径 |
|  ----  | ----  |
| 官方提供的SSD模型  | ` models/ssd/mobilenet_ssd.rknn` |
| 使用1.4版本的RKNN toolkit转换的测试模型([模型结构](https://github.com/xmfbit/warpctc-caffe/blob/master/examples/warpctc_captcha/deploy.prototxt) [模型权重](https://drive.google.com/file/d/0B98MUaCGMMG0UVd1WWFrNHZLdTg/view)) | `models/text_rec/caffe_rec_debug-1.4.rknn` |
| 使用1.6版本的RKNN toolkit转换的测试模型([模型结构](https://github.com/xmfbit/warpctc-caffe/blob/master/examples/warpctc_captcha/deploy.prototxt) [模型权重](https://drive.google.com/file/d/0B98MUaCGMMG0UVd1WWFrNHZLdTg/view))| `models/text_rec/caffe_rec_debug-1.6.rknn` |
