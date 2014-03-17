/*
 * Copyright (C) 2013, 2014 Christoph L. Spiel
 *
 * This file is part of Enblend.
 *
 * Enblend is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Enblend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Enblend; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <algorithm>
#include <cassert>
#include <cstdarg>              // va_list
#include <fstream>              // std::ifstream
#include <iostream>
#include <iterator>             // std::istream_iterator
#include <sstream>
#include <stdexcept>

#include "opencl.h"


namespace ocl
{
#if defined(OPENCL)

    runtime_error::runtime_error(const std::string& a_message) :
        std::runtime_error(a_message),
        opencl_error_(CL_SUCCESS)
    {}


    runtime_error::runtime_error(const cl::Error& an_opencl_error,
                                 const std::string& an_additional_message) :
        std::runtime_error(string_of_error_code(an_opencl_error.err())),
        opencl_error_(an_opencl_error),
        additional_message_(an_additional_message)
    {}


    const cl::Error&
    runtime_error::error() const
    {
        return opencl_error_;
    }


    const std::string&
    runtime_error::additional_message() const
    {
        return additional_message_;
    }


    std::string
    string_of_error_code(cl_int an_error_code)
    {
        switch (an_error_code)
        {
        case CL_SUCCESS: return "success"; // not an error

        case CL_DEVICE_NOT_FOUND: return "device not found";
        case CL_DEVICE_NOT_AVAILABLE: return "device not available";
        case CL_COMPILER_NOT_AVAILABLE: return "compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "memory object allocation failure";
        case CL_OUT_OF_RESOURCES: return "out of resources";
        case CL_OUT_OF_HOST_MEMORY: return "out of host memory";
        case CL_PROFILING_INFO_NOT_AVAILABLE: return "profiling information not available";
        case CL_MEM_COPY_OVERLAP: return "memory copy overlap";
        case CL_IMAGE_FORMAT_MISMATCH: return "image format mismatch";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED: return "image format not supported";
        case CL_BUILD_PROGRAM_FAILURE: return "build program failure";
        case CL_MAP_FAILURE: return "map failure";
        case CL_MISALIGNED_SUB_BUFFER_OFFSET: return "misaligned sub buffer offset";
        case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "exec status error for events in wait list";

        case CL_INVALID_VALUE: return "invalid value";
        case CL_INVALID_DEVICE_TYPE: return "invalid device type";
        case CL_INVALID_PLATFORM: return "invalid platform";
        case CL_INVALID_DEVICE: return "invalid device";
        case CL_INVALID_CONTEXT: return "invalid context";
        case CL_INVALID_QUEUE_PROPERTIES: return "invalid queue properties";
        case CL_INVALID_COMMAND_QUEUE: return "invalid command queue";
        case CL_INVALID_HOST_PTR: return "invalid host pointer";
        case CL_INVALID_MEM_OBJECT: return "invalid memory object";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return "invalid image format descriptor";
        case CL_INVALID_IMAGE_SIZE: return "invalid image size";
        case CL_INVALID_SAMPLER: return "invalid sampler";
        case CL_INVALID_BINARY: return "invalid binary";
        case CL_INVALID_BUILD_OPTIONS: return "invalid build options";
        case CL_INVALID_PROGRAM: return "invalid program";
        case CL_INVALID_PROGRAM_EXECUTABLE: return "invalid program executable";
        case CL_INVALID_KERNEL_NAME: return "invalid kernel name";
        case CL_INVALID_KERNEL_DEFINITION: return "invalid kernel definition";
        case CL_INVALID_KERNEL: return "invalid kernel";
        case CL_INVALID_ARG_INDEX: return "invalid argument index";
        case CL_INVALID_ARG_VALUE: return "invalid argument value";
        case CL_INVALID_ARG_SIZE: return "invalid argument size";
        case CL_INVALID_KERNEL_ARGS: return "invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION: return "invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE: return "invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE: return "invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET: return "invalid global offset";
        case CL_INVALID_EVENT_WAIT_LIST: return "invalid event wait list";
        case CL_INVALID_EVENT: return "invalid event";
        case CL_INVALID_OPERATION: return "invalid operation";
        case CL_INVALID_GL_OBJECT: return "invalid GL object";
        case CL_INVALID_BUFFER_SIZE: return "invalid buffer size";
        case CL_INVALID_MIP_LEVEL: return "invalid MIP level";
        case CL_INVALID_GLOBAL_WORK_SIZE: return "invalid global work size";
        case CL_INVALID_PROPERTY: return "invalid property";

        default:
            std::ostringstream error_code;

            error_code << "unknown error code " << an_error_code;

            return error_code.str();
        }
    }


    static void
    print_platform_info(platform_list_t::const_iterator a_platform, unsigned a_platform_index)
    {
        std::string info;

        a_platform->getInfo(CL_PLATFORM_VENDOR, &info);
        std::cout << "  - Platform #" << a_platform_index << ": " << info;
        a_platform->getInfo(CL_PLATFORM_NAME, &info);
        std::cout << ", " << info;
        a_platform->getInfo(CL_PLATFORM_VERSION, &info);
        std::cout << ", " << info << "\n";
    }


    static void
    print_device_info(device_list_t::const_iterator a_device, unsigned a_device_index)
    {
        std::cout <<
            "    * Device #" << a_device_index << ": max. " <<
            a_device->getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << " work-items\n" <<
            "                 " << a_device->getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / 1024UL <<
            " KB global memory ";

        switch (a_device->getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_TYPE>())
        {
        case CL_NONE: std::cout << "without associated cache";  break;
        case CL_READ_ONLY_CACHE:
            std::cout <<
                "with " <<
                a_device->getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>() / 1024UL <<
                " KB read cache";
            break;
        case CL_READ_WRITE_CACHE:
            std::cout <<
                "with " <<
                a_device->getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>() / 1024UL <<
                " KB read/write cache";
        }

        std::cout << "\n" <<
            "                 " << a_device->getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / 1024UL << " KB " <<
            (a_device->getInfo<CL_DEVICE_LOCAL_MEM_TYPE>() == CL_LOCAL ? "dedicated " : "") <<
            "local memory\n" <<
            "                 " << a_device->getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>() / 1024UL <<
            " KB maximum constant memory\n";
    }


    struct no_platform : runtime_error
    {
        no_platform() : runtime_error("no OpenCL platform found") {}
        explicit no_platform(const std::string& a_message) : runtime_error(a_message) {}
    };


    struct no_device : runtime_error
    {
        no_device() : runtime_error("no OpenCL device found") {}
        explicit no_device(const std::string& a_message) : runtime_error(a_message) {}
    };


    void
    print_opencl_information(bool all_devices)
    {
        platform_list_t platforms;
        cl::Platform::get(&platforms);

        if (platforms.empty())
        {
            std::cout << "  - no platform found\n";
        }
        else
        {
            unsigned platform_index = 1U; // We start enumerating at 1 for user convenience.
            for (auto p = platforms.begin(); p != platforms.end(); ++p, ++platform_index)
            {
                print_platform_info(p, platform_index);

                device_list_t devices;
                try
                {
                    p->getDevices(all_devices ? CL_DEVICE_TYPE_ALL : CL_DEVICE_TYPE_GPU, &devices);
                }
                catch (cl::Error& an_error)
                {
                    // CL_DEVICE_NOT_FOUND is a possible error that
                    // does not hurt as the variable devices will be
                    // empty() then, which is checked below.
                    if (an_error.err() != CL_DEVICE_NOT_FOUND)
                    {
                        throw an_error;
                    }
                }

                if (devices.empty())
                {
                    std::cout <<
                        "    * no " << (all_devices ? "" : "GPU ") <<
                        "devices found on this platform\n";
                }
                else
                {
                    unsigned device_index = 1U; // Again, we start enumerating at 1 for user convenience.
                    for (auto d = devices.begin(); d != devices.end(); ++d, ++device_index)
                    {
                        print_device_info(d, device_index);
                    }
                }
            }
        }
    }


    void
    print_gpu_preference(size_t a_preferred_platform_id, size_t a_preferred_device_id)
    {
        try
        {
            size_t platform_id = a_preferred_platform_id;
            cl::Platform platform;
            device_list_t some_devices;

            platform = find_platform(platform_id);
            prefer_device(platform, a_preferred_platform_id, a_preferred_device_id, some_devices);

            std::cout <<
                "Currently preferred GPU is device #" << a_preferred_device_id <<
                " on platform #" << platform_id <<
                (a_preferred_platform_id == 0U ? " (autodetected)" : "") << ".\n";
        }
        catch (no_platform&)
        {
            std::cout << "No OpenCL platforms found.\n";
        }
        catch (no_device&)
        {
            std::cout << "No OpenCL (GPU) devices found on any platform.\n";
        }
        catch (runtime_error& an_error)
        {
            std::cout <<
                "Platform number #" << a_preferred_platform_id <<
                (a_preferred_platform_id == 0U ? " (autodetected)" : "") <<
                "/device number #" <<
                a_preferred_device_id << " combination is invalid for this system.\n" <<
                an_error.what() << "\n";
        }
    }


    cl::Platform
    find_platform(size_t& a_preferred_platform_id)
    {
        std::ostringstream message;

        platform_list_t platforms;
        try
        {
            cl::Platform::get(&platforms);
        }
        catch (cl::Error& an_error)
        {
            message << "query for OpenCL platforms failed: " << ocl::string_of_error_code(an_error.err());
            throw ocl::runtime_error(message.str());
        }

        if (platforms.empty())
        {
            throw no_platform();
        }
        else
        {
            if (a_preferred_platform_id == 0U)
            {
                ocl::platform_list_t::const_iterator p =
                    std::find_if(platforms.begin(), platforms.end(),
                                 [](const cl::Platform& a_platform)
                                 {
                                     ocl::device_list_t devices;
                                     try {a_platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);}
                                     catch (cl::Error&) {return false;}
                                     return devices.size() >= 1U;
                                 });
                if (p == platforms.end())
                {
                    throw no_device();
                }
                else
                {
                    a_preferred_platform_id = p - platforms.begin() + 1U;
                    return *p;
                }
            }
            else if (a_preferred_platform_id <= platforms.size())
            {
                return platforms[a_preferred_platform_id - 1U];
            }
            else
            {
                message <<
                    "OpenCL platform #" << a_preferred_platform_id <<
                    " is not available; largest OpenCL platform number is " << platforms.size();
                throw ocl::runtime_error(message.str());
            }
        }
    }


    void
    prefer_device(const cl::Platform& a_platform, size_t a_preferred_platform_id,
                  size_t a_preferred_device_id, device_list_t& some_devices)
    {
        std::ostringstream message;

        try
        {
            a_platform.getDevices(CL_DEVICE_TYPE_GPU, &some_devices);
        }
        catch (cl::Error& an_error)
        {
            message <<
                "query for OpenCL GPU devices on platform #" << a_preferred_platform_id + 1U << " failed: " <<
                ocl::string_of_error_code(an_error.err());
            throw ocl::runtime_error(message.str());
        }

        if (some_devices.empty())
        {
            message << "no OpenCL GPU device found on platform #" << a_preferred_platform_id;
            throw no_device(message.str());
        }
        else
        {
            if (a_preferred_device_id <= some_devices.size())
            {
                // move the preferred device in front
                some_devices.insert(some_devices.begin(), some_devices[a_preferred_device_id - 1U]);
                some_devices.erase(some_devices.begin() + a_preferred_device_id);
            }
            else
            {
                message <<
                    "OpenCL device #" << a_preferred_device_id <<
                    " is not available on platform #" << a_preferred_platform_id + 1U <<
                    ", largest device number there is " << some_devices.size();
                throw ocl::runtime_error(message.str());
            }
        }
    }


    static void
    run_self_tests(cl::Context* a_context)
    {
        std::ostringstream message;

        // a_context must be usable.
        std::vector<cl_context_properties> context_properties;
        try
        {
            a_context->getInfo(CL_CONTEXT_PROPERTIES, &context_properties);
        }
        catch (cl::Error& an_error)
        {
            message <<
                "self test failed: cannot query properties of context: " <<
                ocl::string_of_error_code(an_error.err());
            throw ocl::runtime_error(message.str());
        }

        // We need at least one device.
        std::vector<cl::Device> devices;
        try
        {
            a_context->getInfo(CL_CONTEXT_DEVICES, &devices);
        }
        catch (cl::Error& an_error)
        {
            message << "self test failed: cannot query devices in context: " <<
                ocl::string_of_error_code(an_error.err());
            throw ocl::runtime_error(message.str());
        }

        if (devices.empty())
        {
            throw no_device();
        }
    }


    cl::Context*
    create_context(const cl::Platform& a_platform, const device_list_t& some_devices)
    {
        cl_context_properties context_properties[] = {
            CL_CONTEXT_PLATFORM,
            (cl_context_properties) (a_platform)(),
            0
        };
        cl::Context* context = nullptr;

        try
        {
            context = new cl::Context(some_devices, context_properties, nullptr, nullptr);
        }
        catch (cl::Error& an_error)
        {
            std::ostringstream message;
            message << "failed to create OpenCL context: " << ocl::string_of_error_code(an_error.err());
            throw ocl::runtime_error(message.str());
        }

        run_self_tests(context);

        return context;
    }


    ////////////////////////////////////////////////////////////////////////////


    template <class iterator>
    static std::string
    concatenate(const std::string& a_separator, iterator a_begin, iterator an_end)
    {
        if (a_begin == an_end)
        {
            return std::string();
        }
        else
        {
            std::string result(*a_begin);

            while (++a_begin != an_end)
            {
                result.append(a_separator).append(*a_begin);
            }

            return result;
        }
    }


    template <class t, class allocator>
    inline static t*
    data(std::vector<t, allocator>& a_vector)
    {
        return a_vector.empty() ? nullptr : &a_vector[0];
    }


    template <class t, class allocator>
    inline static const t*
    data(const std::vector<t, allocator>& a_vector)
    {
        return a_vector.empty() ? nullptr : &a_vector[0];
    }


    template <typename Enumeration>
    inline static typename std::underlying_type<Enumeration>::type
    as_int(Enumeration a_value)
    {
        return static_cast<typename std::underlying_type<Enumeration>::type>(a_value);
    }


    static std::string
    expand_twiddle(const std::string& a_path)
    {
        std::string result(a_path);
        const char* home = getenv("HOME");

        if (home)
        {
            while (true)
            {
                const std::string::size_type twiddle(result.find('~'));
                if (twiddle == std::string::npos)
                {
                    break;
                }
                result.replace(twiddle, 1U, home);
            }
        }

        return result;
    }


    static std::vector<std::string>
    split_string(const std::string& a_string, char a_delimiter, bool keep_empty_tokens = false)
    {
        std::stringstream s(a_string);
        std::vector<std::string> tokens;
        std::string t;

        while (std::getline(s, t, a_delimiter))
        {
            if (keep_empty_tokens || !t.empty())
            {
                tokens.push_back(t);
            }
        }

        return tokens;
    }


    static std::string
    find_file_in_path(const std::string& a_source_filename, const std::string& a_path,
                      char a_directory_separator = '/', char a_path_separator = ':')
    {
        const std::vector<std::string> directories = split_string(a_path, a_path_separator);

        auto directory =
            std::find_if(directories.begin(), directories.end(),
                         [&a_directory_separator, &a_source_filename](const std::string& a_directory)
                         {
                             const std::string filename(expand_twiddle(a_directory) +
                                                        a_directory_separator + a_source_filename);
                             std::ifstream file(filename.c_str());
                             return file.is_open();
                         });

        if (directory == directories.end())
        {
            return std::string();
        }
        else
        {
            return *directory + a_directory_separator + a_source_filename;
        }
    }


#define OPENCL_PATH "ENBLEND_OPENCL_PATH" //< opencl-path ENBLEND_OPENCL_PATH

// Anticipated Change: Define DEFAULT_OPENCL_PATH via "config.h" which
// in turn gets its input from "configure.in" analogously to RASTER_DIR.
#define DEFAULT_OPENCL_PATH "~/share/enblend/kernels:/usr/share/enblend/kernels"


    static std::string
    find_file(const std::string& a_filename)
    {
        const char a_directory_separator = '/';

        if (a_filename.size() >= 1U && a_filename[0U] == a_directory_separator)
        {
            return a_filename;            // honor absolute path
        }
        else
        {
            // We _always_ search a_filename along of some explicit,
            // given path, never implicitly through CWD or the
            // direcory of the binary.
            std::vector<std::string> paths;

            if (getenv(OPENCL_PATH))
            {
                paths.push_back(getenv(OPENCL_PATH));
            }
            paths.push_back(DEFAULT_OPENCL_PATH);

            for (auto p : paths)
            {
                const std::string f = find_file_in_path(a_filename, p, a_directory_separator);
                if (!f.empty())
                {
                    return f;
                }
            }

            return std::string();
        }
    }


    static std::string
    string_of_variable_arguments(const char *a_format_string, va_list a_variable_argument_list)
    {
        enum struct Limits : size_t {initial_size = 4096, maximum_size = 256 * 4096};

        size_t buffer_size(as_int(Limits::initial_size));
        char* buffer;

        while (true)
        {
            buffer = new char[buffer_size];
            size_t actual_size = vsnprintf(buffer, buffer_size, a_format_string, a_variable_argument_list);

            if (actual_size < buffer_size)
            {
                break;
            }

            delete [] buffer;
            buffer_size *= 2U;

            if (buffer_size > as_int(Limits::maximum_size))
            {
                throw ocl::runtime_error("excessively large vnsprintf buffer");
            }
        }

        std::string result(buffer);
        delete [] buffer;

        return result;
    }


    double
    event_latency(cl::Event& an_event)
    {
        an_event.wait();

        const double start_time = static_cast<double>(an_event.getProfilingInfo<CL_PROFILING_COMMAND_START>());
        const double end_time = static_cast<double>(an_event.getProfilingInfo<CL_PROFILING_COMMAND_END>());

        return 1e-9 * (end_time - start_time);
    }


    void
    check_opencl_event(cl::Event& an_event, const char* a_filename, int a_linenumber)
    {
        try
        {
            const cl_int return_code = an_event.wait();
            if (return_code != CL_SUCCESS)
            {
                std::cerr <<
                    "\n*** CHECK_OPENCL_EVENT failed at " << a_filename << ":" << a_linenumber <<
                    " with code " << return_code <<
                    std::endl;
                exit(1);
            }
        }
        catch (cl::Error& an_error)
        {
            std::cerr <<
                "\n*** CHECK_OPENCL_EVENT raised `" << an_error.what() << "', code `" <<
                string_of_error_code(an_error.err()) << "' at " << a_filename << ":" << a_linenumber <<
                std::endl;
            exit(1);
        }
        catch (...)
        {
            std::cerr <<
                "\n*** CHECK_OPENCL_EVENT threw at " << a_filename << ":" << a_linenumber <<
                std::endl;
            exit(1);
        }
    }


    ////////////////////////////////////////////////////////////////////////////


    std::pair<const char*, size_t>
    SourcePolicy::source()
    {
        return std::make_pair(text().c_str(), text().length() + 1U);
    }


    std::pair<const void*, size_t>
    BinaryPolicy::binary()
    {
        return std::make_pair(static_cast<const void*>(data(code())), code().size());
    }


    SourceStringPolicy::SourceStringPolicy(const std::string& a_source_text) :
        text_(a_source_text)
    {}


    std::string
    SourceStringPolicy::text()
    {
        assert(!text_.empty());
        return text_;
    }


    SourceFilePolicy::SourceFilePolicy(const std::string& a_source_filename) :
        filename_(a_source_filename)
    {}


    std::string
    SourceFilePolicy::text()
    {
        if (text_.empty())
        {
            consult();
            assert(!text_.empty());
        }

        return text_;
    }


    void
    SourceFilePolicy::consult()
    {
        typedef std::istreambuf_iterator<char> file_iterator;

        std::ifstream file(find_file(filename_).c_str());

        if (!file)
        {
            std::ostringstream message;
            message << "OpenCL source-code file not found; missing \"" << filename_ << "\"";
            throw ocl::runtime_error(message.str());
        }
        text_.assign(file_iterator(file), (file_iterator()));

        file.close();
    }


    BinaryCodePolicy::BinaryCodePolicy(const BinaryPolicy::code_t& a_binary_code) :
        code_(a_binary_code)
    {}


    BinaryFilePolicy::BinaryFilePolicy(const std::string& a_binary_filename) :
        filename_(a_binary_filename)
    {}


    BinaryPolicy::code_t
    BinaryFilePolicy::code()
    {
        if (code_.size() == 0U)
        {
            consult();
            assert(code_.size() != 0U);
        }

        return code_;
    }


    void
    BinaryFilePolicy::consult()
    {
        typedef std::istream_iterator<BinaryPolicy::code_t::value_type> file_iterator;

        std::ifstream file(find_file(filename_).c_str());

        if (!file)
        {
            std::ostringstream message;
            message << "OpenCL binary file not found; missing \"" << filename_ << "\"";
            throw ocl::runtime_error(message.str());
        }

        file_iterator iter(file);
        std::copy(iter, file_iterator(), std::back_inserter(code_));

        file.close();
    }


    ////////////////////////////////////////////////////////////////////////////


    template <class actual_code_policy, int default_queue_flags>
    Function<actual_code_policy, default_queue_flags>::Function(const cl::Context& a_context,
                                                                const std::string& a_string) :
        code_policy(a_string),
        context_(a_context),
        devices_(a_context.getInfo<CL_CONTEXT_DEVICES>())
    {
        initialize();
    }


    template <class actual_code_policy, int default_queue_flags>
    void
    Function<actual_code_policy, default_queue_flags>::clear_build_options()
    {
        build_options_.clear();
    }


    template <class actual_code_policy, int default_queue_flags>
    Function<actual_code_policy, default_queue_flags>&
    Function<actual_code_policy, default_queue_flags>::add_build_option(const std::string& an_option)
    {
        build_options_.push_back(an_option);
        return *this;
    }


    template <class actual_code_policy, int default_queue_flags>
    Function<actual_code_policy, default_queue_flags>&
    Function<actual_code_policy, default_queue_flags>::add_build_option(const char *a_format_string, ...)
    {
        va_list argument_pointer;

        va_start(argument_pointer, a_format_string);
        build_options_.push_back(string_of_variable_arguments(a_format_string, argument_pointer));
        va_end(argument_pointer);

        return *this;
    }


    template <class actual_code_policy, int default_queue_flags>
    void
    Function<actual_code_policy, default_queue_flags>::build(const std::string& an_extra_build_option)
    {
        program_ = cl::Program(context_, cl::Program::Sources(1U, code_policy::source()));

        try
        {
            const cl_int error_code UNUSEDVAR =
                program_.build(devices_, build_options(an_extra_build_option).c_str());
#ifndef __CL_ENABLE_EXCEPTIONS
            if (error_code != CL_SUCCESS)
            {
                throw cl::Error(error_code);
            }
#endif
        }
        catch (cl::Error& an_error)
        {
            throw ocl::runtime_error(an_error, build_log());
        }
    }


    template <class actual_code_policy, int default_queue_flags>
    void
    Function<actual_code_policy, default_queue_flags>::build(const char *a_format_string, ...)
    {
        va_list argument_pointer;
        va_start(argument_pointer, a_format_string);

        build(string_of_variable_arguments(a_format_string, argument_pointer));

        va_end(argument_pointer);
    }


    template <class actual_code_policy, int default_queue_flags>
    std::vector<std::string>
    Function<actual_code_policy, default_queue_flags>::build_logs() const
    {
        std::vector<std::string> logs;

        std::transform(devices_.begin(), devices_.end(),
                       std::back_inserter(logs),
                       [this] (cl::Device d) {return program_.getBuildInfo<CL_PROGRAM_BUILD_LOG>(d);});

        return logs;
    }


    template <class actual_code_policy, int default_queue_flags>
    std::string
    Function<actual_code_policy, default_queue_flags>::build_log() const
    {
        return program_.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices_.front());
    }


    template <class actual_code_policy, int default_queue_flags>
    std::vector<BinaryPolicy::code_t>
    Function<actual_code_policy, default_queue_flags>::binaries() const
    {
        std::vector<size_t> sizes(program_.getInfo<CL_PROGRAM_BINARY_SIZES>());
        std::vector<char*> binaries(program_.getInfo<CL_PROGRAM_BINARIES>());
        std::vector<BinaryPolicy::code_t> results;

        results.resize(binaries.size());

        auto s(sizes.begin());
        auto r(results.begin());
        for (auto b = binaries.begin(); b != binaries.end(); ++b, ++s, ++r)
        {
            r->reserve(*s);
            memcpy(&(*r)[0], *b, *s);
        }

        return results;
    }


    template <class actual_code_policy, int default_queue_flags>
    BinaryPolicy::code_t
    Function<actual_code_policy, default_queue_flags>::binary() const
    {
        return binaries().front();
    }


    template <class actual_code_policy, int default_queue_flags>
    const cl::Context&
    Function<actual_code_policy, default_queue_flags>::context() const
    {
        return context_;
    }


    template <class actual_code_policy, int default_queue_flags>
    const std::vector<cl::Device>&
    Function<actual_code_policy, default_queue_flags>::devices() const
    {
        return devices_;
    }


    template <class actual_code_policy, int default_queue_flags>
    const cl::Device&
    Function<actual_code_policy, default_queue_flags>::device() const
    {
        return devices_.front();
    }


    template <class actual_code_policy, int default_queue_flags>
    const cl::Program&
    Function<actual_code_policy, default_queue_flags>::program()
    {
        return program_;
    }


    template <class actual_code_policy, int default_queue_flags>
    cl::Kernel
    Function<actual_code_policy, default_queue_flags>::create_kernel(const std::string& an_entry_point)
    {
        return cl::Kernel(program(), an_entry_point.c_str());
    }


    template <class actual_code_policy, int default_queue_flags>
    std::string
    Function<actual_code_policy, default_queue_flags>::build_options(const std::string& an_extra_build_option) const
    {
        std::string options(concatenate(std::string(" "), build_options_.begin(), build_options_.end()));
        if (!an_extra_build_option.empty())
        {
            options.append(" ").append(an_extra_build_option);
        }

        return options;
    }


    template <class actual_code_policy, int default_queue_flags>
    void
    Function<actual_code_policy, default_queue_flags>::update_program_from_source(const cl::Program::Sources& a_source)
    {
        program_ = cl::Program(context(), a_source);
    }


    template <class actual_code_policy, int default_queue_flags>
    void
    Function<actual_code_policy, default_queue_flags>::initialize()
    {
        for (auto d : devices_)
        {
            queues_.push_back(cl::CommandQueue(context_, d, default_queue_flags));
        }
    }


    template <class actual_code_policy, int default_queue_flags>
    void
    Function<actual_code_policy, default_queue_flags>::finalize()
    {
        for (auto q : queues_)
        {
            q.finish();
        }
    }


    template class Function<SourceStringPolicy>;
    template class Function<SourceFilePolicy>;


    namespace hash
    {
        static std::hash<const char*> function;

        inline static size_t
        of_string(const std::string& a_string)
        {
            return function(a_string.c_str());
        }
    } // namespace hash


    template <class actual_code_policy>
    LazyFunction<actual_code_policy>::LazyFunction(const cl::Context& a_context, const std::string& a_string) :
        super(a_context, a_string),
        build_completed_(false),
        text_hash_(size_t()), build_option_hash_(size_t())
    {}


    template <class actual_code_policy>
    void
    LazyFunction<actual_code_policy>::build(const std::string& an_extra_build_option)
    {
        if (!needs_building(an_extra_build_option))
        {
            return;
        }

        cl::Program::Sources source(1U, code_policy::source());
        super::update_program_from_source(source);

        try
        {
            // Implementation Note: Pass `this' to recover the
            // actual instance when class-static function
            // notify_trampoline() gets called.  The trampoline
            // just invokes method notify().
            super::program().build(std::vector<cl::Device>(1U, super::device()),
                                   super::build_options(an_extra_build_option).c_str(),
                                   notify_trampoline,
                                   this);
            update_hashes(an_extra_build_option);
        }
        catch (cl::Error& an_error)
        {
            cl::Program program(program());
            throw ocl::runtime_error(an_error, program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(super::device()));
        }
    }


    template <class actual_code_policy>
    void
    LazyFunction<actual_code_policy>::notify_trampoline(cl_program a_program, void* an_instance)
    {
        typedef LazyFunction self_t;

        self_t* self = static_cast<self_t*>(an_instance); // Recover pointer to instance.
        self->notify(a_program);
    }


    template <class actual_code_policy>
    void
    LazyFunction<actual_code_policy>::update_hashes(const std::string& an_extra_build_option)
    {
        text_hash_ = hash::of_string(code_policy::text());
        build_option_hash_ = hash::of_string(super::build_options(an_extra_build_option));
    }


    template <class actual_code_policy>
    bool
    LazyFunction<actual_code_policy>::needs_building(const std::string& an_extra_build_option)
    {
        return
            text_hash_ != hash::of_string(code_policy::text()) ||
            build_option_hash_ != hash::of_string(super::build_options(an_extra_build_option));
    }


    template <class actual_code_policy>
    LazyFunctionCXX<actual_code_policy>::LazyFunctionCXX(const cl::Context& a_context,
                                                         const std::string& a_string) :
        super(a_context, a_string)
    {}


    template <class actual_code_policy>
    void
    LazyFunctionCXX<actual_code_policy>::wait()
    {
        std::unique_lock<std::mutex> lock(build_completed_mutex_);

        // Anticipated Change: Replace the following lines with
        //     build_completed_condition_.wait(lock, super::build_completed())
        // when g++ does not fail with an ICE anymore.
        while (!super::build_completed())
        {
            build_completed_condition_.wait(lock);
        }
    }


    template <class actual_code_policy>
    bool
    LazyFunctionCXX<actual_code_policy>::build_completed()
    {
        std::lock_guard<std::mutex> lock(build_completed_mutex_);

        return super::build_completed();
    }


    template <class actual_code_policy>
    void
    LazyFunctionCXX<actual_code_policy>::notify(cl_program a_program UNUSEDVAR)
    {
        build_completed_mutex_.lock();
        super::set_build_completed(true);
        build_completed_mutex_.unlock();
        build_completed_condition_.notify_all();
    }


    template class LazyFunctionCXX<SourceStringPolicy>;
    template class LazyFunctionCXX<SourceFilePolicy>;


    void
    BatchBuilder::submit(value_t a_function, const char *a_format_string, ...)
    {
        va_list argument_pointer;

        va_start(argument_pointer, a_format_string);
        submit(a_function, string_of_variable_arguments(a_format_string, argument_pointer));
        va_end(argument_pointer);
    }


    void
    SerialBatchBuilder::submit(value_t a_function, const std::string& a_build_option)
    {
        if (a_function)
        {
            a_function->build(a_build_option);
            a_function->wait();
        }
#ifdef DEBUG
        else
        {
            std::cerr << "+ SerialBatchBuilder::submit: silently ignoring null-function\n";
        }
#endif
    }


    ThreadedBatchBuilder::ThreadedBatchBuilder() : run_(true)
    {
        std::thread builder(build_all_trampoline, this);
        builder.detach();
    }


    ThreadedBatchBuilder::~ThreadedBatchBuilder()
    {
        finalize();
    }


    void
    ThreadedBatchBuilder::submit(value_t a_function, const std::string& a_build_option)
    {
        assert(run_);

        if (a_function)
        {
            std::unique_lock<std::recursive_mutex> lock(queue_mutex_);
            compile_queue_.push_front(BuildCommand(a_function, a_build_option));
            queue_not_empty_.notify_one();
        }
#ifdef DEBUG
        else
        {
            std::cerr << "+ ThreadedBatchBuilder::submit: silently ignoring null-function\n";
        }
#endif
    }


    void
    ThreadedBatchBuilder::finalize()
    {
        run_ = false;
    }


    void
    ThreadedBatchBuilder::build_all_trampoline(ThreadedBatchBuilder* self)
    {
        self->build_all();
    }


    void
    ThreadedBatchBuilder::build()
    {
        queue_mutex_.lock();
        if (compile_queue_.empty())
        {
#ifdef DEBUG
            std::cerr << "+ ThreadedBatchBuilder::build -- spurious wake-up?\n";
#endif
            queue_mutex_.unlock();
            return;
        }

        BuildCommand build_command(compile_queue_.back());
        compile_queue_.pop_back();
        queue_mutex_.unlock();

        build_command.function->build(build_command.option);
        build_command.function->wait();
    }


    void
    ThreadedBatchBuilder::build_all()
    {
        while (run_)
        {
            std::unique_lock<std::recursive_mutex> lock(queue_mutex_);
            while (compile_queue_.empty())
            {
                queue_not_empty_.wait(lock);
            }

            build();
            queue_mutex_.unlock();
        }
    }

#else

#endif // OPENCL
} // namespace ocl
