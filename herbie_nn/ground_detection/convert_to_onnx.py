'''import torch
import torch.onnx

model = torch.load('semnasnet_100-.0074-80out.pt')
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
                  output_names = ['output'], # the model's output names
                  dynamic_axes={'input' : {0 : 'batch_size'},    # variable length axes
                                'output' : {0 : 'batch_size'}})

'''
import onnx
import onnx_tensorrt.backend as backend
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

