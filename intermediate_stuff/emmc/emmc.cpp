#include "emmc.hpp"
#include "sd.h"

namespace hw
{

Rpi_Emmc::Rpi_Emmc()
{
    static int counter = 0;
    id_ = counter++;
    sd_init();
}

Rpi_Emmc::~Rpi_Emmc() {}

std::string Rpi_Emmc::device_name()
{
    return "emmc1";
}

const char *Rpi_Emmc::driver_name()
{
    return "emmc1";
}

block_t Rpi_Emmc::size()
{
    return (block_t)128 * 1024 * 1024 * 1024 / block_size();
}

block_t Rpi_Emmc::block_size()
{
    return 512;
}

void Rpi_Emmc::read(block_t blk, size_t count, on_read_func reader)
{
    // todo
}

buffer_t Rpi_Emmc::read_sync(block_t blk, size_t count = 1)
{
    unsigned char *buf = (unsigned char *)malloc(count * block_size());
    sd_readblock(blk, buf, count);
    return fs::construct_buffer(buf, buf + count * block_size());
}

void Rpi_Emmc::write(block_t blk, buffer_t buf, on_write_func)
{
    // todo
}

bool Rpi_Emmc::write_sync(block_t blk, buffer_t buf)
{
    sd_writeblock(buf->data(), blk);
}

void Rpi_Emmc::deactivate() override {}

} // namespace hw