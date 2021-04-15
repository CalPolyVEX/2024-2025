import torch
import torch.onnx
import onnx
from onnx import helper
import onnxruntime
import numpy as np
import sys, cv2

#import onnx_tensorrt.backend as backend

def convert_pytorch_onnx(model_name = 'efficientnet_lite0-.0102-80out.pt'):
    #model_name = 'efficientnet_lite0-.0102-80out.pt'
    #model_name = 'mobilenetv3_small-.0110-80out.pt'
    #semnasnet_100-.0074-80out.pt'
    print ('Converting ' + model_name + ' to .onnx')
    model = torch.load(model_name)
    model.eval()

    print (model)

    x = torch.rand(3,360,640, requires_grad = True)
    x = x.unsqueeze(0)
    x = x.to('cuda:0')

    #print(model(x))
    batch_size = 1
    # Export the model
    torch.onnx.export(model,                     # model being run
                    x,                         # model input (or a tuple for multiple inputs)
                    "test_model.onnx",         # where to save the model (can be a file or file-like object)
                    export_params=True,        # store the trained parameter weights inside the model file
                    opset_version=11,          # the ONNX version to export the model to
                    do_constant_folding=True,  # whether to execute constant folding for optimization
                    input_names = ['input'],   # the model's input names
                    output_names = ['output']) # the model's output names
                    # dynamic_axes={'input' : {0 : 'batch_size'},    # variable length axes
                    #                 'output' : {0 : 'batch_size'}})

def onnx_inference():
    layers = []
    max_val = []
    min_val = []
    model_name = 'test_model.onnx'
    new_model_name = 'test_mod_model.onnx'
    model = onnx.load(model_name)

    # Check that the IR is well formed
    onnx.checker.check_model(model)

    # Print a human readable representation of the graph
    #onnx.helper.printable_graph(model.graph)

    #print layer names
    for node in model.graph.node:
        print(str(node.name) + ':  ' + str(node.output))

        if str(node.output[0]) != 'output':
            intermediate_layer_value_info = helper.ValueInfoProto()
            intermediate_layer_value_info.name = str(node.output[0])
            model.graph.output.extend([intermediate_layer_value_info])

    onnx.save(model,new_model_name)

    print ('Running onnx inference')
    img = cv2.imread('beach.jpg').astype(np.float32) / 255
    ximg = cv2.resize(img, (640, 360), 0, 0, interpolation = cv2.INTER_LINEAR)
    ximg = np.expand_dims(ximg, axis=0)
    ximg = np.transpose(ximg, (0,3,1,2))
    print (ximg.shape)
    # return
    # ximg = np.random.rand(1, 3, 360, 640).astype(np.float32)
    sess = onnxruntime.InferenceSession(new_model_name)

    for x in sess.get_outputs():
        if x.name not in layers:
            if x.name != 'output':
                layers.append(x.name)

        #print(x.name)
        #print(x.shape)

    print("The model input shape: ", sess.get_inputs()[0].shape)
    print("The model output shape: ", sess.get_outputs()[0].shape)
    print("The shape of the Image is: ", ximg.shape)
    #print (layers)

    input_name = sess.get_inputs()[0].name
    label_name = sess.get_outputs()[0].name
    result = sess.run(None, {input_name: ximg})
    prob = result[0]
    #print(prob.ravel()[:80])
    print ('layers len: ' + str(len(layers)))
    print ('output len: ' + str(len(result)))

    for x in range(len(result)-1):
        #print (result[x])
        min = np.amin(result[x+1])
        max = np.amax(result[x+1])
        output_name = sess.get_outputs()[x+1].name
        print ('layer: ' + layers[x] + output_name + '  min ' + str(min) + '  max: ' + str(max))
        max_val.append(np.amax(result[x+1]))
        min_val.append(np.amin(result[x+1]))
    #print (result)

def onnx_tensorrt_inference():
    import onnx
    import numpy as np
    import time

    model = onnx.load("test_model.onnx")
    engine = backend.prepare(model, device='CUDA:0')
    input_data = np.random.random(size=(1, 3, 360, 640)).astype(np.float32)
    for x in range(2):
        print ('iteration: ' + str(x))
        start = time.time()
        print (input_data)
        output_data = engine.run(input_data)[0]
        print (input_data)
        end = time.time()
        print(str(1000.0/(end - start)))

def main(args):
   if len(args) == 2:
       convert_pytorch_onnx(args[1])
   onnx_inference()

if __name__ == '__main__':
   main(sys.argv)

