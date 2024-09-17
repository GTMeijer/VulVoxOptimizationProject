#include "pch.h"
#include "vulkan_window.h"


//TODO: Probably do some lambda stuff here?
void Vulkan_Window::main_loop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        draw_frame();
    }

    //Wait until all operations are completed before cleanup
    vkDeviceWaitIdle(device);
}

void Vulkan_Window::draw_frame()
{
    vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphores[current_frame], nullptr, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        //Swap chain in not compatible with the current window size, recreate
        recreate_swap_chain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    //Reset fence *after* confirming the swapchain is valid (prevents deadlock)
    vkResetFences(device, 1, &in_flight_fences[current_frame]);

    vkResetCommandBuffer(command_buffers[current_frame], 0);
    record_command_buffer(command_buffers[current_frame], image_index);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //Which semaphore to wait for before execution and at which stage
    std::array<VkSemaphore, 1> wait_semaphores = { image_available_semaphores[current_frame] };
    std::array<VkPipelineStageFlags, 1> wait_stages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = wait_semaphores.size();
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_stages.data();

    //Which semaphore to signal when the command buffer is done
    std::array<VkSemaphore, 1> signal_semaphores = { render_finished_semaphores[current_frame] };
    submit_info.signalSemaphoreCount = signal_semaphores.size();
    submit_info.pSignalSemaphores = signal_semaphores.data();

    //Link command buffer
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[current_frame];

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores.data(); //Wait for semaphore before we can present

    std::array<VkSwapchainKHR, 1> swap_chains = { swap_chain };
    present_info.swapchainCount = swap_chains.size();
    present_info.pSwapchains = swap_chains.data();
    present_info.pImageIndices = &image_index;

    //Dont need to collect the draw results because the present function returns is as well
    present_info.pResults = nullptr;

    result = vkQueuePresentKHR(present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized)
    {
        framebuffer_resized = false;

        //Swap chain in not compatible with the current window size, recreate
        recreate_swap_chain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }


    //Rotate to next frame resources
    current_frame = (current_frame++) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan_Window::init_window()
{
    std::cout << "Init window.." << std::endl;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

    //Pass pointer of this to glfw so we can retrieve the object instance in the callback function
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

    std::cout << "Window initialized." << std::endl;
}

void Vulkan_Window::init_vulkan()
{
    std::cout << "Init vulkan.." << std::endl;

    create_instance();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_pool();
    create_vertex_buffer();
    create_command_buffer();
    create_sync_objects();

    std::cout << "Vulkan initialized." << std::endl;
}

void Vulkan_Window::cleanup()
{
    cleanup_swap_chain();

    vkDestroyBuffer(device, vertex_buffer, nullptr);
    vkFreeMemory(device, vertex_buffer_memory, nullptr);

    vkDestroyPipeline(device, graphics_pipeline, nullptr);

    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

    vkDestroyRenderPass(device, render_pass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device, image_available_semaphores.at(i), nullptr);
        vkDestroySemaphore(device, render_finished_semaphores.at(i), nullptr);
        vkDestroyFence(device, in_flight_fences.at(i), nullptr);
    }

    //Also destroys the command buffers
    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void Vulkan_Window::cleanup_swap_chain()
{
    for (auto& framebuffer : swap_chain_framebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto& image_view : swap_chain_image_views)
    {
        vkDestroyImageView(device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(device, swap_chain, nullptr);
}

/// <summary>
/// Create a vulkan instance and connect it to a glfw window
/// </summary>
void Vulkan_Window::create_instance()
{
    if (enableValidationLayers && !check_validation_layer_support())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;

    if (enableValidationLayers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }

    if (!check_glfw_extension_support())
    {
        throw std::runtime_error("Not all glfw extensions are supported!");
    }

    if (VkResult result = vkCreateInstance(&create_info, nullptr, &instance); result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance! " + std::string(string_VkResult(result)));
    }

    //if VK_ERROR_INCOMPATIBLE_DRIVER on mac, check: https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/01_Instance.html#_encountered_vk_error_incompatible_driver

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::cout << "Available extensions: \n";

    for (const auto& extension : extensions)
    {
        std::cout << '\t' << extension.extensionName << '\n';
    }

    std::cout << std::endl;
}

void Vulkan_Window::create_surface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void Vulkan_Window::create_logical_device()
{
    Queue_Family_Indices indices = find_queue_families(physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

    //Set queue priority, higher values have higher priority
    float queue_priority = 1.0f;

    //Setup the information needed to create the queues.
    for (uint32_t queue_family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1; //Only one queue is needed, multiple threads can each have a command buffer that feeds into it
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    //Set the required device features
    VkPhysicalDeviceFeatures device_features{}; //No required features for now.

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    //Pass the queue information
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();

    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();

    if (enableValidationLayers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }

    //Setup device specific extensions
        //Nothing for now.

    if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device!");
    }

    //Store the handles for the graphics and present queues
    vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);

    vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

void Vulkan_Window::create_swap_chain()
{
    //Query supported formats and extends

    Swap_Chain_Support_Details swap_chain_support = query_swap_chain_support(physical_device);

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
    std::cout << "Surface format: " << string_VkFormat(surface_format.format) << std::endl;

    VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
    std::cout << "Present mode: " << string_VkPresentModeKHR(present_mode) << std::endl;

    VkExtent2D extent = choose_swap_extent(swap_chain_support.capabilities);
    std::cout << "Image buffer extent: " << extent.width << " x " << extent.height << std::endl;

    std::cout << std::endl;

    //Add one to the image buffer count so we have another image buffer to write to when the driver is swapping image buffers
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

    //Make sure we dont exceed the maximum image buffer count
    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
    {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;

    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1; //Always one, unless we are working on a stereoscopic 3D application
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //We are writing directly to the image buffer, change when adding post-processing

    Queue_Family_Indices indices = find_queue_families(physical_device);

    std::array<uint32_t, 2> queue_family_indices = { indices.graphics_family.value(), indices.present_family.value() };


    if (indices.graphics_family != indices.present_family)
    {
        //If the graphics and present queues are seperate we default to exclusive mode for simplicity
        //This saves us handling ownership of the images
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform; //Rotates or flips the image in the swapchain (disabled)
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //Disable window alpha blend
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE; //Don't render obscure pixels
    create_info.oldSwapchain = nullptr; //For now, required when recreating the swap chain (e.g. when minimizing)

    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }

    //Store the handles to the images inside the swap chains so we can write to them
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;
}

void Vulkan_Window::create_image_views()
{
    swap_chain_image_views.resize(swap_chain_images.size());

    for (size_t i = 0; i < swap_chain_images.size(); i++)
    {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain_images[i];

        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swap_chain_image_format;

        //Default color channel mapping
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        //Single layer image, no mipmapping
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &create_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void Vulkan_Window::create_render_pass()
{
    //The render pass describes the framebuffer attachements 
    //and how many color and depth buffers there are
    //and how their content should be handled

    VkAttachmentDescription color_attachment{};
    color_attachment.format = swap_chain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; //No multisampling
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //Clear buffer before rendering
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //Store rendered content

    //No stencil buffer operations yet, so dont care about the data
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    //This buffer is used for presentation
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    //Index of fragment shader out_color
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    //Setup the dependencies between the subpasses (cant start b before a)
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    //Wait until the swap chain finished reading
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    //Color write has to wait until swap chain is done
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass!");
    }





}

void Vulkan_Window::create_graphics_pipeline()
{
    //Load shader files and compile to SPIR-V code
    std::filesystem::path vert_shader_filepath("shaders/vert.spv");
    std::filesystem::path frag_shader_filepath("shaders/frag.spv");

    auto vert_shader_code = read_file(vert_shader_filepath);
    std::cout << "Vertex shader file read, size is " << vert_shader_code.size() << " bytes" << std::endl;

    if (std::filesystem::file_size(vert_shader_filepath) != vert_shader_code.size())
    {
        throw std::runtime_error("Failed to load vertex shader, size missmatch!");
    }

    auto frag_shader_code = read_file(frag_shader_filepath);
    std::cout << "Fragment shader file read, size is " << frag_shader_code.size() << " bytes" << std::endl;

    if (std::filesystem::file_size(frag_shader_filepath) != frag_shader_code.size())
    {
        throw std::runtime_error("Failed to load fragment shader, size missmatch!");
    }

    std::cout << std::endl;

    //Wrap the shader byte code in the shader modules for use in the pipeline
    VkShaderModule vert_shader_module = create_shader_module(vert_shader_code);
    VkShaderModule frag_shader_module = create_shader_module(frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";
    vert_shader_stage_info.pSpecializationInfo = nullptr; //Use this for control flow flags

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";
    frag_shader_stage_info.pSpecializationInfo = nullptr; //Use this for control flow flags

    //Some parts of the pipeline can be dynamic, we define these parts here
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages_info = { vert_shader_stage_info, frag_shader_stage_info };

    //Describe the format of the vertex data
    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();


    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description; //Optional, spacing between data and per vertex or per instance
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data(); //Optional, attribute type, which bindings to load, and offset

    //Describes the configuration of the vertices the triangles and lines use
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //Triangle from every three vertices, without reuse
    input_assembly_info.primitiveRestartEnable = VK_FALSE; //Break up lines and triangles in strip mode

    //Viewport size
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swap_chain_extent.width;
    viewport.height = (float)swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    //Visible part of the viewport (whole viewport)
    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = swap_chain_extent;

    //It is usefull to have a non-static viewport and scissor size
    std::vector<VkDynamicState> dynamic_states =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    VkPipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount = 1;

    //Setup rasterizer stage
    VkPipelineRasterizationStateCreateInfo rasterizer_info{};
    rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.depthClampEnable = VK_FALSE; //Discard fragments outside of near/far plane (instead of clamping)
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE; //If true, discards all geometery
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL; //Full render, switch for wireframe or points (among others)
    rasterizer_info.lineWidth = 1.0f; //wider requires wideLines GPU feature
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT; //Backface culling
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE; //Clockwise vertex order determines the front
    rasterizer_info.depthBiasEnable = VK_FALSE;
    rasterizer_info.depthBiasConstantFactor = 0.0f;
    rasterizer_info.depthBiasClamp = 0.0f;
    rasterizer_info.depthBiasSlopeFactor = 0.0f;

    //Multisample / anti-aliasing stage (disabled)
    VkPipelineMultisampleStateCreateInfo multisampling_info{};
    multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_info.minSampleShading = 1.0f; // Optional
    multisampling_info.pSampleMask = nullptr; // Optional
    multisampling_info.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling_info.alphaToOneEnable = VK_FALSE; // Optional

    //Handle color blending of the fragments (for example alpha blending) (disabled)
    VkPipelineColorBlendAttachmentState color_blend_attachement_info{};
    color_blend_attachement_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachement_info.blendEnable = VK_FALSE; //Disabled
    color_blend_attachement_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
    color_blend_attachement_info.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
    color_blend_attachement_info.colorBlendOp = VK_BLEND_OP_ADD; //Optional
    color_blend_attachement_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
    color_blend_attachement_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
    color_blend_attachement_info.alphaBlendOp = VK_BLEND_OP_ADD; //Optional

    VkPipelineColorBlendStateCreateInfo color_blending_info{};
    color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_info.logicOpEnable = VK_FALSE; //Disabled
    color_blending_info.logicOp = VK_LOGIC_OP_COPY; //Optional
    color_blending_info.attachmentCount = 1;
    color_blending_info.pAttachments = &color_blend_attachement_info;
    color_blending_info.blendConstants[0] = 0.0f; //Optional
    color_blending_info.blendConstants[1] = 0.0f; //Optional
    color_blending_info.blendConstants[2] = 0.0f; //Optional
    color_blending_info.blendConstants[3] = 0.0f; //Optional

    //Define global variables (like a transformation matrix) (none for now)
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0; //Optional uniform data
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 0; //Optional small uniform data
    pipeline_layout_info.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2; //Vert & Frag shader stages
    pipeline_info.pStages = shader_stages_info.data();

    //Shader stages
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blending_info;
    pipeline_info.pDynamicState = &dynamic_state_info;

    //Fixed function stage
    pipeline_info.layout = pipeline_layout;

    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;

    //This pipeline doesn't derive from another 
    //(set VK_PIPELINE_CREATE_DERIVATIVE_BIT, if we want to derive from another)    
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }



    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    vkDestroyShaderModule(device, vert_shader_module, nullptr);


}

/// <summary>
/// Creates the framebuffers that references the data to the attachments (e.g. image resources)
/// </summary>
void Vulkan_Window::create_framebuffers()
{
    swap_chain_framebuffers.resize(swap_chain_image_views.size());

    for (size_t i = 0; i < swap_chain_image_views.size(); i++)
    {
        std::array<VkImageView, 1> attachments = { swap_chain_image_views[i] };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = swap_chain_extent.width;
        framebuffer_info.height = swap_chain_extent.height;
        framebuffer_info.layers = 1; //Only single layer images in the swap chain

        if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swap_chain_framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

/// <summary>
/// Creates the command pool that manages the memory that stores the buffers
/// </summary>
void Vulkan_Window::create_command_pool()
{
    Queue_Family_Indices queue_family_indices = find_queue_families(physical_device);

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

    if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool!");
    }
}

/// <summary>
/// Creates the memory buffer containing our vertex data
/// </summary>
void Vulkan_Window::create_vertex_buffer()
{
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
    create_buffer(buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertex_buffer,
        vertex_buffer_memory);

    void* data;
    vkMapMemory(device, vertex_buffer_memory, 0, buffer_size, 0, &data);

    memcpy(data, vertices.data(), (size_t)buffer_size);

    vkUnmapMemory(device, vertex_buffer_memory);
}

void Vulkan_Window::create_command_buffer()
{
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //Directly submits to command queue (secondary can be used by primary)
    alloc_info.commandBufferCount = (uint32_t)command_buffers.size(); //Double buffer for concurrent frame processing

    if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void Vulkan_Window::create_sync_objects()
{
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Start in signaled state for first frame

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create semaphores!");
        }
    }
}

void Vulkan_Window::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only used by graphics queue
    buffer_info.flags = 0;

    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    //Query the physical device requirements to use the vertex buffer
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    //The memory has to be writable from the CPU side
    //Also, ensure the mapped memory matches the content of the allocated memory (no flush needed)
    allocate_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocate_info, nullptr, &buffer_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    //Bind the buffer to the allocated memory
    vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void Vulkan_Window::record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

    //attach this render pass to the swap chain image
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swap_chain_framebuffers[image_index];

    //Cover the whole swap chain image
    render_pass_info.renderArea.offset = { 0,0 };
    render_pass_info.renderArea.extent = swap_chain_extent;

    //Clear to black (we use VK_ATTACHMENT_LOAD_OP_CLEAR)
    VkClearValue clear_color = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    //VK_SUBPASS_CONTENTS_INLINE means we don't use secondary command buffers
    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    //Bind to graphics pipeline (not compute)
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    //We set viewport and scissor to dynamic earlier, so we define them now
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_chain_extent.width);
    viewport.height = static_cast<float>(swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    //Set the vertex buffers
    std::array<VkBuffer, 1>  vertex_buffers = { vertex_buffer };
    std::array<VkDeviceSize, 1> offsets = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, vertex_buffers.size(), vertex_buffers.data(), offsets.data());

    //Draw command, set vertex and instance counts and indices
    vkCmdDraw(command_buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

/// <summary>
/// In case of a window resize we need to recreate the swap chain so it is compatible again
/// </summary>
void Vulkan_Window::recreate_swap_chain()
{
    int new_width = 0;
    int new_height = 0;
    glfwGetFramebufferSize(window, &new_width, &new_height);

    while (new_width == 0 || new_height == 0) {
        glfwGetFramebufferSize(window, &new_width, &new_height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanup_swap_chain();

    create_swap_chain();
    create_image_views(); //Depend on swap chain
    create_framebuffers(); //Depend on image views
}

/// <summary>
/// Select the GPU to use among available devices.
/// </summary>
void Vulkan_Window::pick_physical_device()
{
    uint32_t device_count = 0;

    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0)

    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());


    std::multimap<int, VkPhysicalDevice> device_candidates;

    std::cout << "Found devices:\n";
    int i = 1;
    for (auto& device : devices)
    {
        int device_rating = rate_physical_device(device);

        std::cout << i << ": " << get_physical_device_name(device) << "\t" << get_physical_device_type(device) << "\t" << "Score: " << device_rating << "\n";

        device_candidates.insert(std::make_pair(rate_physical_device(device), device));

        i++;
    }

    std::cout << std::endl;

    //Use the highest scoring GPU. If score is 0, no suitable GPU was found.
    if (device_candidates.rbegin()->first > 0)
    {
        physical_device = device_candidates.rbegin()->second;
    }
    else
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

VkSurfaceFormatKHR Vulkan_Window::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) const
{
    for (const auto& available_format : available_formats)
    {
        //We prefer the SRGB format
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return available_format;
        }
    }

    //If our preferred format is not available just use the first available one
    return available_formats[0];
}

VkPresentModeKHR Vulkan_Window::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const
{
    for (const auto& available_present_mode : available_present_modes)
    {
        //We prefer triple buffering
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return available_present_mode;
        }
    }

    //We default to vertical sync
    return VK_PRESENT_MODE_FIFO_KHR;
}

/// <summary>
/// Pick the appropriate resolution for the swap chain.
/// </summary>
/// <param name="capabilities"></param>
/// <returns></returns>
VkExtent2D Vulkan_Window::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        //Use the recommended (by vulkan) pixel size
        return capabilities.currentExtent;
    }
    else
    {
        //Retrieve the window size in pixels and clamp to the available swap chain size
        //Allows support for screens with high DPI

        int width;
        int height;

        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actual_extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actual_extent;
    }
}

std::string Vulkan_Window::get_physical_device_name(const VkPhysicalDevice& device) const
{
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    std::string device_name(device_properties.deviceName);

    return device_name;
}

std::string Vulkan_Window::get_physical_device_type(const VkPhysicalDevice& device) const
{
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    switch (device_properties.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "VK_PHYSICAL_DEVICE_TYPE_CPU";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}

/// <summary>
/// Rate a given GPU device based on application requirements.
/// </summary>
/// <param name="device"></param>
/// <returns></returns>
int Vulkan_Window::rate_physical_device(const VkPhysicalDevice& device) const
{
    int device_score = 0;

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    //Discrete GPUs perform better than intergrated, fall back to virtual if none are available
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        device_score += 1000;
    }
    else if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        device_score += 100;
    }
    else if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
    {
        device_score += 1;
    }

    device_score += device_properties.limits.maxImageDimension2D;

    Queue_Family_Indices indices = find_queue_families(device);

    bool extensions_supported = check_device_extension_support(device);

    bool swap_chain_adequate = false;
    if (extensions_supported)
    {
        Swap_Chain_Support_Details swap_chain_support = query_swap_chain_support(device);
        swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
    }

    if (!indices.is_complete() && !extensions_supported && !swap_chain_adequate)
    {
        return 0;
    }

    return device_score;
}

/// <summary>
/// Checks if all the required extensions needed by glfw are available.
/// </summary>
bool Vulkan_Window::check_glfw_extension_support() const
{
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::vector<std::string> available_extension_names;
    for (const auto& extension : extensions)
    {
        available_extension_names.emplace_back(extension.extensionName);
    }

    std::cout << "Required extensions for glfw:\n";

    bool all_supported = true;
    for (uint32_t i = 0; i < glfw_extension_count; i++)
    {
        std::string glfw_extension_name = glfw_extensions[i];

        std::cout << "\t" << glfw_extension_name << "\t";

        if (contains(available_extension_names, glfw_extension_name))
        {
            std::cout << "AVAILABLE\n";
        }
        else
        {
            all_supported = false;
            std::cout << "UNAVAILABLE\n";
        }
    }

    std::cout << std::endl;

    return all_supported;
}

bool Vulkan_Window::check_device_extension_support(const VkPhysicalDevice& device) const
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string, std::less<>> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : available_extensions)
    {
        required_extensions.erase(extension.extensionName);
    }

    //If the required extension set is empty, all required extensions are supported by this device.
    return required_extensions.empty();
}

bool Vulkan_Window::check_validation_layer_support() const
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char* layer_name : validation_layers)
    {
        bool layer_found = true;

        for (const auto& layerProperties : available_layers)
        {
            if (strcmp(layer_name, layerProperties.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
        {
            return false;
        }

    }

    return true;
}

Vulkan_Window::Swap_Chain_Support_Details Vulkan_Window::query_swap_chain_support(const VkPhysicalDevice& device) const
{
    Swap_Chain_Support_Details details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());

    if (format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

    if (present_mode_count != 0)
    {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
    }

    return details;
}

Vulkan_Window::Queue_Family_Indices Vulkan_Window::find_queue_families(const VkPhysicalDevice& device) const
{
    Queue_Family_Indices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queue_family : queue_families)
    {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphics_family = i;
        }

        //Check if this device supports window system integration (a.k.a. presentation)
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (present_support)
        {
            indices.present_family = i;
        }

        //Early out if all queue families are found.
        if (indices.is_complete())
        {
            break;
        }

        i++;
    }


    return indices;
}

VkShaderModule Vulkan_Window::create_shader_module(const std::vector<char>& bytecode)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = bytecode.size();
    create_info.pCode = reinterpret_cast<const uint32_t*> (bytecode.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }


    return shader_module;
}

/// <summary>
/// Find a suitable memory type available on the physical device, choosing from one of the given suitable types
/// </summary>
/// <param name="type_filter">Suitable memory types</param>
/// <param name="properties">Properties the memory type needs to support</param>
/// <returns></returns>
uint32_t Vulkan_Window::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if (type_filter & (1 << i) && //Memory type has to by one of suitable types
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) //Memory has to support all required properties
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}
