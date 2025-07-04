load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("//tensorflow/lite:build_def.bzl", "tflite_copts")
load("//tensorflow/lite/tools/evaluation/tasks:build_def.bzl", "task_linkopts")

package(
    # copybara:uncomment default_applicable_licenses = ["//tensorflow:license"],
    default_visibility = [
        "//visibility:public",
    ],
    licenses = ["notice"],
)
cc_library(
     name = "fpga_ip_driver",
     srcs = [
         "fpga_ip_driver.cc",
     ],
     hdrs = [
         "fpga_ip_driver.h",
     ],
     alwayslink = 1,
     visibility = ["//visibility:public"],
)

cc_library(
    name = "add_fpga_test_delegate",
    srcs = [
        "add_fpga_test_delegate.cc",
    ],
    hdrs = [
        "add_fpga_test_delegate.h",
    ],
    deps = [
        ":fpga_ip_driver",
        "//tensorflow/lite/c:c_api_types",
        "//tensorflow/lite/c:common",
        "//tensorflow/lite/core/c:common",
        "//tensorflow/lite/delegates/utils:simple_delegate",

        # Additional deps for internal kernels/utilities
        "//tensorflow/lite:framework",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite/kernels/internal:compatibility",
    ],
    alwayslink = 1,
)



cc_binary(
    name = "add_fpga_test_external_delegate.so",
    srcs = [
        "external_delegate_adaptor.cc",
    ],
    deps = [
        ":add_fpga_test_delegate",
        ":fpga_ip_driver",
        "//tensorflow/lite/delegates/external:external_delegate_interface",
        "//tensorflow/lite/tools:command_line_flags",
        "//tensorflow/lite/tools:logging",
    ],
    
    copts = ["-fPIC"],
    linkshared = 1,
    linkstatic = 1,
    visibility = ["//visibility:public"],
)

sh_test(
    name = "external_delegate_test",
    srcs = ["external_delegate_test.sh"],
    data = [
        "//tensorflow/lite/delegates/utils/mw_delegates/add_fpga_test_delegate:add_fpga_test_external_delegate.so",
        "//tensorflow/lite/tools/benchmark:benchmark_model",
        "@tflite_mobilenet_float//:mobilenet_v1_1.0_224.tflite",
    ],
    visibility = ["//visibility:private"],
)

