/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "paddle/fluid/operators/grid_sampler_op.h"
#include "paddle/fluid/framework/op_registry.h"
#ifdef PADDLE_WITH_CUDA
#include "paddle/fluid/platform/cudnn_helper.h"
#endif

namespace paddle {
namespace operators {

using Tensor = framework::Tensor;

class GridSampleOp : public framework::OperatorWithKernel {
  public:
    using framework::OperatorWithKernel::OperatorWithKernel;
    void InferShape(framework::InferShapeContext* ctx) const override {
      PADDLE_ENFORCE(ctx->HasInput("X"),
                    "Input(X) of GridSampleOp should not be null.");
      PADDLE_ENFORCE(ctx->HasInput("Grid"),
                    "Input(Grid) of GridSampleOp should not be null.");
      PADDLE_ENFORCE(ctx->HasOutput("Output"),
                    "Output(Output) of GridSampleOp should not be null.");
      
      auto x_dims = ctx->GetInputDim("X");
      auto grid_dims = ctx->GetInputDim("Grid");
      PADDLE_ENFORCE(x_dims.size() == 4, "Input(X) of GridSampleOp should be 4-D Tensor.");
      PADDLE_ENFORCE(grid_dims.size() == 4, "Input(Grid) of GridSampleOp should be 4-D Tensor.");
      PADDLE_ENFORCE(grid_dims[3] == 2, "Input(Grid) dims[3] should be 2.");
      PADDLE_ENFORCE_EQ(grid_dims[0], x_dims[0], "Input(X) and Input(Grid) dims[0] should be equal.");
      PADDLE_ENFORCE_EQ(grid_dims[1], x_dims[2], "Input(X) dims[2] and Input(Grid) dims[1] should be equal.");
      PADDLE_ENFORCE_EQ(grid_dims[2], x_dims[3], "Input(X) dims[3] and Input(Grid) dims[2] should be equal.");

      ctx->SetOutputDim("Output", x_dims);
      ctx->ShareLoD("X", "Output");
    }
  
  protected:
    framework::OpKernelType GetExpectedKernelType(
        const framework::ExecutionContext& ctx) const override {
      framework::LibraryType library_{framework::LibraryType::kPlain};
#ifdef PADDLE_WITH_CUDA
      if (platform::CanCUDNNBeUsed(ctx)) {
        library_ = framework::LibraryType::kCUDNN;
      }
#endif    
      return framework::OpKernelType(
          framework::ToDataType(ctx.Input<Tensor>("X")->type()),
          ctx.GetPlace(), framework::DataLayout::kAnyLayout, library_);
    }
};

class GridSampleOpMaker : public framework::OpProtoAndCheckerMaker {
  public:
    void Make() override {
      AddInput(
          "X",
          "(Tensor) The input tensor of GridSampleOp, "
          "This is a 4-D tensor with shape of [N, C, H, W]");
      AddInput(
          "Grid",
          "(Tensor) The output of AffineGridOp, "
          "This is a 4-D tensor with shape of [N, H, W, 2]");
      AddOutput(
          "Output",
          "(Tensor) Output tensor with shape [N, C, H, W]");
      AddAttr<bool>(
          "use_cudnn",
          "(bool, default false) Only used in cudnn kernel, need install cudnn")
          .SetDefault(true);

      AddComment(R"DOC(
      It sample input X by grid gennerate by AffineGridOp.
      )DOC");
    }
};

class GridSampleOpGrad : public framework::OperatorWithKernel {
  public:
  using framework::OperatorWithKernel::OperatorWithKernel;
  void InferShape(framework::InferShapeContext* ctx) const override {
    //TO DO
  }

  protected:
    framework::OpKernelType GetExpectedKernelType(
        const framework::ExecutionContext& ctx) const override {
      framework::LibraryType library_{framework::LibraryType::kPlain};
#ifdef PADDLE_WITH_CUDA
      if (platform::CanCUDNNBeUsed(ctx)) {
        library_ = framework::LibraryType::kCUDNN;
      }
#endif    
      return framework::OpKernelType(
          framework::ToDataType(ctx.Input<Tensor>("X")->type()),
          ctx.GetPlace(), framework::DataLayout::kAnyLayout, library_);
    }
};

class GridSampleGradMaker : public framework::SingleGradOpDescMaker {
  public:
    using framework::SingleGradOpDescMaker::SingleGradOpDescMaker;

  protected:
    std::unique_ptr<framework::OpDesc> Apply() const override {
      auto* op = new framework::OpDesc();
      op->SetType("grid_sampler_grad");
      op->SetInput("X", Input("X"));
      op->SetInput("Grid", Input("Grid"));
      op->SetInput(framework::GradVarName("Output"), OutputGrad("Output"));

      op->SetAttrMap(Attrs());

      op->SetOutput(framework::GradVarName("X"), InputGrad("X"));
      op->SetOutput(framework::GradVarName("Grid"), InputGrad("Grid"));
      return std::unique_ptr<framework::OpDesc>(op);
    }
};

} // namespace operators
} // namespace paddle

namespace ops = paddle::operators;
REGISTER_OPERATOR(grid_sampler, ops::GridSampleOp, ops::GridSampleOpMaker,
                  ops::GridSampleGradMaker);
REGISTER_OPERATOR(grid_sampler_grad, ops::GridSampleOpGrad);

REGISTER_OP_CPU_KERNEL(
    grid_sampler,
    ops::GridSampleOpKernel<paddle::platform::CPUDeviceContext, float>,
    ops::GridSampleOpKernel<paddle::platform::CPUDeviceContext, double>);
REGISTER_OP_CPU_KERNEL(
    grid_sampler_grad,
    ops::GridSampleGradOpKernel<paddle::platform::CPUDeviceContext, float>,
    ops::GridSampleGradOpKernel<paddle::platform::CPUDeviceContext, double>);
