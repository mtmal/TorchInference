////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023 Mateusz Malinowski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#include "TorchInference.h"

TorchInference::TorchInference(const int width, const int height) : mWidth(width), mHeight(height)
{
}

TorchInference::~TorchInference()
{
}

void TorchInference::initialise(const char* pathToModel)
{
    /** Empty image */
    cv::Mat dummy(cv::Size(mWidth, mHeight), CV_8UC3, cv::Scalar(0));

    /** Load the model */
    mModule = torch::jit::load(pathToModel);
    /* Upload model to GPU */
    mModule.to(torch::kCUDA);
    /* Invoke processImage once to fully initialise the module. Reuse mInputTensor */
    processImage(dummy, mInputTensor);
}

torch::Tensor& TorchInference::processImage(const cv::Mat& inputImage, torch::Tensor& output)
{
    inputImage.convertTo(mFloatImage, CV_32FC3, 1.0f / 255.0f);
    mInputTensor = torch::from_blob(mFloatImage.data, {1, mHeight, mWidth, 3});
    mInputTensor = mInputTensor.permute({0, 3, 1, 2});
    /* do the shenanigans */
    mInputTensor[0][0] = mInputTensor[0][0].sub_(0.485).div_(0.229);
    mInputTensor[0][1] = mInputTensor[0][1].sub_(0.456).div_(0.224);
    mInputTensor[0][2] = mInputTensor[0][2].sub_(0.406).div_(0.225);
    mInputTensor = mInputTensor.to(torch::kCUDA);
    output = mModule.forward({mInputTensor.to(torch::kFloat16)}).toTensor().detach().to(torch::kCPU);
    return output;
}
