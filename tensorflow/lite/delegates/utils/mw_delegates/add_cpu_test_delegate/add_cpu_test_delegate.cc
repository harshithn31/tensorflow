#include "tensorflow/lite/delegates/utils/mw_delegates/add_cpu_test_delegate/add_cpu_test_delegate.h"

#include <memory>
#include <utility>
#include <vector>

#include "tensorflow/lite/c/c_api_types.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/delegates/utils/simple_delegate.h"
#include "tensorflow/lite/kernels/internal/tensor.h"
// #include "tensorflow/lite/kernels/internal/tensor_utils.h"
#include "tensorflow/lite/builtin_ops.h"

namespace tflite {
namespace add_cpu_test {

// AddCpuTestDelegateKernel implements the interface of SimpleDelegateKernelInterface.
// This holds the Delegate capabilities.
class AddCpuTestDelegateKernel : public SimpleDelegateKernelInterface {
 public:
  explicit AddCpuTestDelegateKernel(const AddCpuTestDelegateOptions& options)
      : options_(options) {}

  TfLiteStatus Init(TfLiteContext* context,
                    const TfLiteDelegateParams* params) override {
    // Save index to all nodes which are part of this delegate.
    inputs_.resize(params->nodes_to_replace->size);
    outputs_.resize(params->nodes_to_replace->size);
    builtin_code_.resize(params->nodes_to_replace->size);
    for (int i = 0; i < params->nodes_to_replace->size; ++i) {
      const int node_index = params->nodes_to_replace->data[i];
      // Get this node information.
      TfLiteNode* delegated_node = nullptr;
      TfLiteRegistration* delegated_node_registration = nullptr;
      TF_LITE_ENSURE_EQ(
          context,
          context->GetNodeAndRegistration(context, node_index, &delegated_node,
                                          &delegated_node_registration),
          kTfLiteOk);
      inputs_[i].push_back(delegated_node->inputs->data[0]);
      inputs_[i].push_back(delegated_node->inputs->data[1]);
      outputs_[i].push_back(delegated_node->outputs->data[0]);
      builtin_code_[i] = delegated_node_registration->builtin_code;
    }
    return !options_.error_during_init ? kTfLiteOk : kTfLiteError;
  }

  TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) override {
    return !options_.error_during_prepare ? kTfLiteOk : kTfLiteError;
  }

  TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) override {
    // Evaluate the delegated graph.
    // Here we loop over all the delegated nodes.
    // We know that all the nodes are either ADD or SUB operations and the
    // number of nodes equals ''inputs_.size()'' and inputs[i] is a list of
    // tensor indices for inputs to node ''i'', while outputs_[i] is the list of
    // outputs for node
    // ''i''. Note, that it is intentional we have simple implementation as this
    // is for demonstration.

    for (int i = 0; i < inputs_.size(); ++i) {
      // Get the node input tensors.
      // Add/Sub operation accepts 2 inputs.
      auto& input_tensor_1 = context->tensors[inputs_[i][0]];
      auto& input_tensor_2 = context->tensors[inputs_[i][1]];
      auto& output_tensor = context->tensors[outputs_[i][0]];
      TF_LITE_ENSURE_EQ(
          context,
          ComputeResult(context, builtin_code_[i], &input_tensor_1,
                        &input_tensor_2, &output_tensor),
          kTfLiteOk);
    }

    return !options_.error_during_invoke ? kTfLiteOk : kTfLiteError;
  }

 private:
  const AddCpuTestDelegateOptions options_;
  std::vector<std::vector<int>> inputs_, outputs_;
  std::vector<int> builtin_code_;

  TfLiteStatus ComputeResult(TfLiteContext* context, int builtin_code,
                             const TfLiteTensor* input_tensor_1,
                             const TfLiteTensor* input_tensor_2,
                             TfLiteTensor* output_tensor) {
    if (NumElements(input_tensor_1->dims) != NumElements(input_tensor_2->dims) ||
        NumElements(input_tensor_1->dims) != NumElements(output_tensor->dims)) {
      return kTfLiteDelegateError;
    }
    // This code assumes no activation, and no broadcasting needed (both inputs
    // have the same size).
    auto* input_1 = GetTensorData<int32>(input_tensor_1);
    auto* input_2 = GetTensorData<int32>(input_tensor_2);
    auto* output = GetTensorData<int32>(output_tensor);
    for (int i = 0; i < NumElements(input_tensor_1->dims); ++i) {
      if (builtin_code == kTfLiteBuiltinAdd)
        output[i] = input_1[i] + input_2[i];
      else
        output[i] = input_1[i] - input_2[i];
    }
    return kTfLiteOk;
  }

  int NumElements(const TfLiteIntArray* dims) {
    int count = 1;
    for (int i = 0; i < dims->size; ++i) {
      count *= dims->data[i];
    }
    return count;
  }
};

class AddCpuTestDelegate : public SimpleDelegateInterface {
 public:
  explicit AddCpuTestDelegate(const AddCpuTestDelegateOptions& options)
      : options_(options) {}

  bool IsNodeSupportedByDelegate(const TfLiteRegistration* registration,
                                 const TfLiteNode* node,
                                 TfLiteContext* context) const override {
    // This delegate supports only ADD and SUB operations.
    if (registration->builtin_code != kTfLiteBuiltinAdd &&
        registration->builtin_code != kTfLiteBuiltinSub)
      return false;
    // Only supports int32 type
    for (int i = 0; i < node->inputs->size; ++i) {
      auto& tensor = context->tensors[node->inputs->data[i]];
      if (tensor.type != kTfLiteInt32) {
        return false;
      }
    }
    for (int i = 0; i < node->outputs->size; ++i) {
      auto& tensor = context->tensors[node->outputs->data[i]];
      if (tensor.type != kTfLiteInt32) {
        return false;
      }
    }
    return true;
  }
  TfLiteStatus Initialize(TfLiteContext* context) override { return kTfLiteOk; }

  const char* Name() const override {
    static constexpr char kName[] = "AddCpuTestDelegate";
    // This name is used for debugging/logging/profiling.
    // It is important to use a unique name for each delegate.
    // If multiple delegates have the same name, it can lead to confusion in
    return kName;
  }

  std::unique_ptr<SimpleDelegateKernelInterface> CreateDelegateKernelInterface()
      override {
    return std::make_unique<AddCpuTestDelegateKernel>(options_);
  }

  SimpleDelegateInterface::Options DelegateOptions() const override {
    // Use default options.
    return SimpleDelegateInterface::Options();
  }

 private:
  const AddCpuTestDelegateOptions options_;
};

}  // namespace add_cpu_test
}  // namespace tflite

AddCpuTestDelegateOptions TfLiteAddCpuTestDelegateOptionsDefault() {
  AddCpuTestDelegateOptions options = {0};
  // Just assign an invalid builtin code so that this dummy test delegate will
  // not support any node by default.
  options.allowed_builtin_code = -1;
  return options;
}

// Creates a new delegate instance that need to be destroyed with
// `TfLiteAddCpuTestDelegateDelete` when delegate is no longer used by TFLite.
// When `options` is set to `nullptr`, the above default values are used:
TfLiteDelegate* TfLiteAddCpuTestDelegateCreate(const AddCpuTestDelegateOptions* options) {
  std::unique_ptr<tflite::add_cpu_test::AddCpuTestDelegate> add_cpu_test_delegate(
      new tflite::add_cpu_test::AddCpuTestDelegate(
          options ? *options : TfLiteAddCpuTestDelegateOptionsDefault()));
  return tflite::TfLiteDelegateFactory::CreateSimpleDelegate(std::move(add_cpu_test_delegate));
}

// Destroys a delegate created with `TfLiteAddCpuTestDelegateCreate` call.
void TfLiteAddCpuTestDelegateDelete(TfLiteDelegate* delegate) {
  tflite::TfLiteDelegateFactory::DeleteSimpleDelegate(delegate);
}



