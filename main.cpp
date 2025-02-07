#include <iostream>
#include <libenvpp/env.hpp>

#include "3dgs/Renderer.h"
#include "args.hxx"
#include "spdlog/spdlog.h"

int main(int argc, char** argv) {
    spdlog::set_pattern("[%H:%M:%S] [%^%L%$] %v");

    args::ArgumentParser parser("Vulkan Splatting");
    args::HelpFlag helpFlag{parser, "help", "Display this help menu", {'h', "help"}};
    args::Flag validationLayersFlag{
        parser, "validation-layers", "Enable Vulkan validation layers", {"validation"}
    };
    args::Flag verboseFlag{parser, "verbose", "Enable verbose logging", {'v', "verbose"}};
    args::ValueFlag<uint8_t> physicalDeviceIdFlag{
        parser, "physical-device", "Select physical device by index", {'d', "device"}
    };
    args::Flag immediateSwapchainFlag{
        parser, "immediate-swapchain", "Set swapchain mode to immediate (VK_PRESENT_MODE_IMMEDIATE_KHR)",
        {'i', "immediate-swapchain"}
    };
    args::Flag noGuiFlag{parser, "no-gui", "Disable GUI", { "no-gui"}};
    args::Positional<std::string> scenePath{parser, "scene", "Path to scene file", "scene.ply"};

    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Completion& e) {
        std::cout << e.what();
        return 0;
    }
    catch (const args::Help&) {
        std::cout << parser;
        return 0;
    }
    catch (const args::ParseError& e) {
        std::cout << e.what() << std::endl;
        std::cout << parser;
        return 1;
    }

    auto pre = env::prefix("VKGS");
    auto validationLayers = pre.register_variable<bool>("VALIDATION_LAYERS");
    auto physicalDeviceId = pre.register_variable<uint8_t>("PHYSICAL_DEVICE");
    auto immediateSwapchain = pre.register_variable<bool>("IMMEDIATE_SWAPCHAIN");
    auto envVars = pre.parse_and_validate();

    if (args::get(verboseFlag)) {
        spdlog::set_level(spdlog::level::debug);
    }

    RendererConfiguration config{
        envVars.get_or(validationLayers, false),
        envVars.get(physicalDeviceId).has_value()
            ? std::make_optional(envVars.get(physicalDeviceId).value())
            : std::nullopt,
        envVars.get_or(immediateSwapchain, false),
        args::get(scenePath)
    };

    // check that the scene file exists
    if (!std::filesystem::exists(config.scene)) {
        spdlog::critical("File does not exist: {}", config.scene);
        return 0;
    }

    if (validationLayersFlag) {
        config.enableVulkanValidationLayers = args::get(validationLayersFlag);
    }

    if (physicalDeviceIdFlag) {
        config.physicalDeviceId = std::make_optional<uint8_t>(args::get(physicalDeviceIdFlag));
    }

    if (immediateSwapchainFlag) {
        config.immediateSwapchain = args::get(immediateSwapchainFlag);
    }

    if (noGuiFlag) {
        config.enableGui = false;
    }

#ifndef DEBUG
    try {
#endif
    auto renderer = Renderer(config);
    renderer.initialize();
    renderer.run();
#ifndef DEBUG
    } catch (const std::exception& e) {
        spdlog::critical(e.what());
        std::cout << e.what() << std::endl;
        return 0;
    }
#endif
    return 0;
}
