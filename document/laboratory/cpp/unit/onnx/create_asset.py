import torch
import torch.nn as nn

class MatrixAdd(nn.Module):
    def forward(self, x):
        # Simple model logic: Add the input matrix to itself
        return x + x

model = MatrixAdd()
model.eval()

# Generate dummy data (2x2 matrix) to define the tensor shape
dummy_input = torch.randn(2, 2)

# Export the model to ONNX format
torch.onnx.export(
    model, 
    dummy_input, 
    "matrix_add.onnx", 
    input_names=["input"], 
    output_names=["output"],
    # Allow dynamic batch sizes for flexible evaluation
    dynamic_axes={"input": {0: "batch_size"}, "output": {0: "batch_size"}}
)

print("Successfully generated matrix_add.onnx!")