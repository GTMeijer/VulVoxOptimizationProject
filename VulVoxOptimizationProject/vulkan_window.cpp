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

    //Update global variables (camera etc.)
    update_uniform_buffer(current_frame);

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

    //Dont need to collect the draw results because the present function returns them as well
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

void Vulkan_Window::load_model()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> unique_vertices{};

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            //tiny obj loader packs the vertices as three floats, so multiply index by three
            vertex.position =
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            //Same for texture coordinates, but two instead
            vertex.texture_coordinates =
            {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1] //Flip the vertical axis for vulkan standard
            };

            vertex.color = { 1.0, 1.0, 1.0f };

            vertices.push_back(vertex);
            //indices.push_back(indices.size());

            if (unique_vertices.count(vertex) == 0)
            {
                unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(unique_vertices[vertex]);
        }
    }

    std::cout << "Model loaded containing " << vertices.size() << " vertices." << std::endl;

}
void Vulkan_Window::update_uniform_buffer(uint32_t current_image)
{
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    //Rotate around the z-axis
    MVP mvp{};
    mvp.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f)) * glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f));
    mvp.view = glm::lookAt(glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(0.0f, 0.0f, 15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mvp.projection = glm::perspective(glm::radians(45.0f), (float)swap_chain_extent.width / (float)swap_chain_extent.height, 0.1f, 100.0f);

    //mvp.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    //mvp.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //mvp.projection = glm::perspective(glm::radians(45.0f), (float)swap_chain_extent.width / (float)swap_chain_extent.height, 0.1f, 10.0f);

    mvp.projection[1][1] *= -1; //Invert y-axis so its compatible with Vulkan axes

    memcpy(uniform_buffers_mapped[current_image], &mvp, sizeof(mvp));
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
    create_descriptor_set_layout();
    create_graphics_pipeline();
    create_command_pool();
    create_depth_resources();
    create_framebuffers();
    create_texture_image();
    create_texture_image_view();
    create_texture_sampler();
    load_model();
    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffer();
    create_sync_objects();

    std::cout << "Vulkan initialized." << std::endl;
}

void Vulkan_Window::cleanup()
{
    cleanup_swap_chain();

    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(device, uniform_buffers[i], nullptr);
        vkFreeMemory(device, uniform_buffers_memory[i], nullptr);
    }

    //Descriptor sets will be destroyed with the pool
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

    vkDestroySampler(device, texture_sampler, nullptr);
    vkDestroyImageView(device, texture_image_view, nullptr);

    vkDestroyImage(device, texture_image, nullptr);
    vkFreeMemory(device, texture_image_memory, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

    vkDestroyBuffer(device, index_buffer, nullptr);
    vkFreeMemory(device, index_buffer_memory, nullptr);

    vkDestroyBuffer(device, vertex_buffer, nullptr);
    vkFreeMemory(device, vertex_buffer_memory, nullptr);

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
    vkDestroyImageView(device, depth_image_view, nullptr);
    vkDestroyImage(device, depth_image, nullptr);
    vkFreeMemory(device, depth_image_memory, nullptr);

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
    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE; //Device needs to support anisotropic filtering

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
        swap_chain_image_views[i] = create_image_view(swap_chain_images[i], swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Vulkan_Window::create_render_pass()
{
    //The render pass describes the framebuffer attachments 
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

    //Describe how the depth buffer is handled
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = find_depth_format();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //We dont care what the gpu does with the data after determining the depth
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //We dont care about previous depth content
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //Attach the color and depth buffers to the single subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    //Setup the dependencies between the subpasses (cant start b before a)
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    //Wait until the swap chain finished reading before writing a new image
    //Also wait with writing a new depth buffer until the previous is done being read
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    //Attach the color and depth attachements to the render pass
    std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

/// <summary>
/// Creates the descriptor sets that describe the layout of the data buffers in our pipeline.
/// Some hardware only allows up to four descriptor sets being created,
/// so instead of allocating a seperate set for each resource we bind them together into one.
/// </summary>
void Vulkan_Window::create_descriptor_set_layout()
{
    //Layout binding for the uniform buffer
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0; //Same as in shader
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //This buffer is (only) referenced in the vertex stage
    ubo_layout_binding.pImmutableSamplers = nullptr; //Optional

    //Layout binding for the image sampler
    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //Connect to fragment shader stage

    //Bind the layout bindings in the single descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to created descriptor set layout!");
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
    rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //Counter clockwise vertex order determines the front (we flip the y axis)
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

    //Enable depth testing
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE; //Discard fragment is they fail depth test
    depth_stencil.depthWriteEnable = VK_TRUE; //Write passed depth tests to the depth buffer
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS; //Write closer results to the depth buffer
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.minDepthBounds = 0.0f; // Optional
    depth_stencil.maxDepthBounds = 1.0f; // Optional
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = {}; // Optional
    depth_stencil.back = {}; // Optional

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
    pipeline_layout_info.setLayoutCount = 1; //Amount of uniform buffers
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
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
    pipeline_info.pDepthStencilState = &depth_stencil;
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
        //The swap chain uses multiple images, the render pipeline uses a single depth buffer (protected by semaphores)
        std::array<VkImageView, 2> attachments = { swap_chain_image_views[i], depth_image_view };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
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

void Vulkan_Window::create_depth_resources()
{
    VkFormat depth_format = find_depth_format();

    create_image(
        swap_chain_extent.width, swap_chain_extent.height,
        depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depth_image,
        depth_image_memory);

    depth_image_view = create_image_view(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);



}

void Vulkan_Window::create_texture_image()
{
    int texture_width;
    int texture_height;
    int texture_channels;

    //Load texture image, force alpha channel
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
    VkDeviceSize image_size = texture_width * texture_height * 4;

    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image!");
    }

    //Setup host visible staging buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

    //Copy the texture data into the staging buffer
    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(device, staging_buffer_memory);

    stbi_image_free(pixels);

    //Texels == pixels

    create_image(texture_width, texture_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image, texture_image_memory);

    //Change layout of target image memory to be optimal for writing destination
    transition_image_layout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //Transfer the image data from the staging buffer to the image memory
    copy_buffer_to_image(staging_buffer, texture_image, static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height));
    //Change layout of image memory to be optimal for reading by a shader
    transition_image_layout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);
}

void Vulkan_Window::create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory)
{
    //Descripe image memory format
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1; //No mip mapping
    image_info.arrayLayers = 1;
    image_info.format = format; //Same as pixel buffers
    image_info.tiling = tiling; //We dont need to access the images memory so no need for linear tiling
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //Discard after first transition
    image_info.usage = usage; //Destination for buffer copy and readable by shader
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Only one queue family will use the image (graphics)
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0; //Optional

    if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image!");
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, image, &memory_requirements);

    //Allocate memory for the texture image
    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocate_info, nullptr, &image_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, image_memory, 0);
}

void Vulkan_Window::transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    //Create barrier to prevent reading before write is done
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;

    //Queue family indices for ownership transfer, we dont want to do this here so set IGNORED
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0; //No mipmapping
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0; //No array
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        //Transfer to write optimal layout
        barrier.srcAccessMask = 0; //No need to wait
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; //

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //Start immediately (start of pipeline)
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        //Transfer write optimal to optimal shader read layout
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    //Send command for image barrier
    vkCmdPipelineBarrier(command_buffer,
        source_stage,
        destination_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    end_single_time_commands(command_buffer);
}

void Vulkan_Window::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0; //Start of pixel values
    region.bufferRowLength = 0; //Memory layout
    region.bufferImageHeight = 0; //Memory layout

    //Part of the image we want to copy
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0,0,0 };
    region.imageExtent = { width, height, 1 };

    //Enqueue the image copy operation
    vkCmdCopyBufferToImage(
        command_buffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    end_single_time_commands(command_buffer);
}

VkImageView Vulkan_Window::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;

    //Default color channel mapping (SWIZZLE to default)

    //Single layer image, no mipmapping
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image view!");
    }

    return image_view;
}

void Vulkan_Window::create_texture_image_view()
{
    texture_image_view = create_image_view(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

}

void Vulkan_Window::create_texture_sampler()
{
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    //Magnification and minification interpolation.
    //Nearest is pixelated, linear is smooth
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;

    //How to handle when the surface is larger than the texture
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    //What color to use when ADDRESS MODE is set to border clamp
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    //Use 0..1 if false or 0..TexWidth/0..TexHeight if true for texture coordinates
    sampler_info.unnormalizedCoordinates = VK_FALSE;

    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

    //No mip mapping at the moment, for later?
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    if (vkCreateSampler(device, &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

/// <summary>
/// Creates the memory buffer containing our vertex data
/// </summary>
void Vulkan_Window::create_vertex_buffer()
{
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    //Create staging buffer that transfers data between the host and device
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer,
        staging_buffer_memory);

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, vertices.data(), (size_t)buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);

    //Create vertex buffer as device only buffer
    create_buffer(buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertex_buffer,
        vertex_buffer_memory);

    //Copy data from host to device
    copy_buffer(staging_buffer, vertex_buffer, buffer_size);

    //Data on device, cleanup temp buffers
    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);
}

void Vulkan_Window::create_index_buffer()
{
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer,
        staging_buffer_memory);

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, indices.data(), (size_t)buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);

    //Create index buffer as device only buffer
    create_buffer(buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        index_buffer,
        index_buffer_memory);

    //Copy data from host to device
    copy_buffer(staging_buffer, index_buffer, buffer_size);

    //Data on device, cleanup temp buffers
    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);
}

void Vulkan_Window::create_uniform_buffers()
{
    VkDeviceSize buffer_size = sizeof(MVP);

    uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffers[i], uniform_buffers_memory[i]);

        //Map pointer to memory so we can write to the buffer, this pointer is persistant throughout the applications lifetime
        vkMapMemory(device, uniform_buffers_memory[i], 0, buffer_size, 0, &uniform_buffers_mapped[i]);
    }
}

void Vulkan_Window::create_descriptor_pool()
{
    std::array<VkDescriptorPoolSize, 2> pool_sizes{};

    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    pool_info.flags = 0;

    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void Vulkan_Window::create_descriptor_sets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);

    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocate_info.pSetLayouts = layouts.data();

    descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocate_info, descriptor_sets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    //Describe the data layout inside the buffers we want to write
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniform_buffers[i];
        buffer_info.offset = 0;
        buffer_info.range = sizeof(MVP);

        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = texture_image_view;
        image_info.sampler = texture_sampler;

        std::array<VkWriteDescriptorSet, 2> descriptor_writes{};

        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = descriptor_sets[i]; //Binding to update
        descriptor_writes[0].dstBinding = 0; //Binding index equal to shader binding index
        descriptor_writes[0].dstArrayElement = 0; //Index of array data to update, no array so zero
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1; //A single buffer info struct
        descriptor_writes[0].pBufferInfo = &buffer_info; //Data and layout of buffer
        descriptor_writes[0].pImageInfo = nullptr; //Optional, only used for image data
        descriptor_writes[0].pTexelBufferView = nullptr; //Optional, only used for buffers views (for tex buffers)

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = descriptor_sets[i]; //Binding to update
        descriptor_writes[1].dstBinding = 1; //Binding index equal to shader binding index
        descriptor_writes[1].dstArrayElement = 0; //Index of array data to update, no array so zero
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[1].descriptorCount = 1; //A single buffer info struct
        descriptor_writes[1].pImageInfo = &image_info; //This is an image sampler so we provide an image data description

        //Apply updates
        //Only write descriptor, no copy, so copy variable is 0
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
    }
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

/// <summary>
/// Creates a temporary command buffer that transfers data between scr and dst buffers, executed immediately
/// </summary>
/// <param name="src_buffer"></param>
/// <param name="dst_buffer"></param>
/// <param name="size"></param>
void Vulkan_Window::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    //Record copy
    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0; // Optional
    copy_region.dstOffset = 0; // Optional
    copy_region.size = size;

    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    end_single_time_commands(command_buffer);
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
    //Clear order should be same as attachment order
    std::array<VkClearValue, 2> clear_colors{};
    clear_colors[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } }; //Clear image to black
    clear_colors[1].depthStencil = { 1.0f, 0 }; //Clear depth image to 1.0 (far plane)
    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_colors.size());
    render_pass_info.pClearValues = clear_colors.data();

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

    //Set the index buffers
    vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

    //Set the uniform buffers
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[current_frame], 0, nullptr);

    //Draw command, set vertex and instance counts (we're not using instancing) and indices
    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

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
    create_depth_resources(); //Depend on depth image
    create_framebuffers(); //Depend on image views
}

VkCommandBuffer Vulkan_Window::begin_single_time_commands()
{
    //Setup temp buffer for transfer

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);

    //Start recording command buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //We only use this buffer once

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

void Vulkan_Window::end_single_time_commands(VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    //Execute copy command buffer instantly and wait until transfer is complete
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
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

    //Use the highest scoring GPU. If score is 0, no suitable GPU was found.
    if (device_candidates.rbegin()->first > 0)
    {
        physical_device = device_candidates.rbegin()->second;
        std::cout << get_physical_device_name(physical_device) << " selected." << std::endl;
    }
    else
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    std::cout << std::endl;
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

    //Anisotropic filtering is required, check capability using the physical device features struct
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    if (!indices.is_complete() && !extensions_supported && !swap_chain_adequate && !supportedFeatures.samplerAnisotropy)
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

bool Vulkan_Window::has_stencil_component(VkFormat format) const
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

/// <summary>
/// Check the physical device for supported depth formats.
/// </summary>
/// <returns></returns>
VkFormat Vulkan_Window::find_depth_format()
{
    return find_supported_format
    (
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat Vulkan_Window::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}
