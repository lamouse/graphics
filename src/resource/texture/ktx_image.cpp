#include "ktx_image.hpp"
#include <stdexcept>
#include <filesystem>
#include "common/file.hpp"
#if _WIN32
#define WIN32_LEAN_AND_MEAN
#endif
#include <boost/process.hpp>

namespace {
void check_ktx_error(KTX_error_code code) {
    if ((code != KTX_SUCCESS)) {
        throw std::runtime_error(ktxErrorString(code));
    }
}
}  // namespace
namespace resource::image {
KtxImage::KtxImage(const std::string& path) {
    check_ktx_error(ktxTexture_CreateFromNamedFile(
        path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &handle));
}
KtxImage::~KtxImage() {
    if (handle) {
        ktxTexture_Destroy(handle);
    }
}

auto createKtxImage(std::string_view path, std::string_view dstDir) -> std::string {
    std::filesystem::path src_path(path);
    std::filesystem::path dst_path(dstDir);
    src_path.replace_extension(".ktx2");
    common::FS::create_dir(dst_path);
    dst_path /= src_path.filename();
    namespace bp = boost::process;
    boost::asio::io_context context;
    auto c = bp::environment::current();
    // we need to use a value, since windows needs wchar_t.
    std::vector<bp::environment::key_value_pair> my_env{c.begin(), c.end()};
    my_env.emplace_back("KTX_TOOL_PATH=./tools");
    auto exe = bp::environment::find_executable("ktx", my_env);

    bp::process proc(context, exe,
                     {"create", "--generate-mipmap", "--format", "B8G8R8A8_UNORM", "--assign-tf",
                      "srgb", path.data(), dst_path.generic_string()},
                     bp::process_environment(my_env));
    if (proc.wait() != 0) {
        throw std::runtime_error("wait ktx process!");
    }
    if (proc.exit_code() != 0) {
        throw std::runtime_error("ktx process execute error!");
    }
    return dst_path.string();
}

auto createCubeMapKtxImage(std::span<std::string_view, 6> images, std::string_view name,
                           std::string_view dstDir) -> std::string {
    std::filesystem::path dst_path(dstDir);
    common::FS::create_dir(dst_path);
    dst_path /= (std::string(name) + ".ktx2");
    namespace bp = boost::process;
    boost::asio::io_context context;
    auto c = bp::environment::current();
    // we need to use a value, since windows needs wchar_t.
    std::vector<bp::environment::key_value_pair> my_env{c.begin(), c.end()};
    my_env.emplace_back("KTX_TOOL_PATH=./tools");
    auto exe = bp::environment::find_executable("ktx", my_env);

    bp::process proc(
        context, exe,
        {"create", "--generate-mipmap", "--format", "R8G8B8A8_SRGB", "--cubemap", "--assign-tf",
         "srgb", images[0].data(), images[1].data(), images[2].data(), images[3].data(),
         images[4].data(), images[5].data(), dst_path.generic_string()},
        bp::process_environment(my_env));
    if (proc.wait() != 0) {
        throw std::runtime_error("wait ktx process!");
    }
    if (proc.exit_code() != 0) {
        throw std::runtime_error("ktx process execute error!");
    }

    return dst_path.string();
}

}  // namespace resource::image