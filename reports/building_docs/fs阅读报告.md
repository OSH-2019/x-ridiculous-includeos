希望大家吧vscode中的include_path变量配好，然后看代码和这个
所有头文件都在 includeos/api/下




# dirent
貌似是一个文件的基础类型
描述了一个文件的类型（FILE，DIR，VOLUME——ID，INVALID——ENTITY）；文件的文件系统（即是fat32下的一个文件还是fat16）（const File_system* fs_;）；文件名；文件所在的block；文件的双亲（parent，“Parent's block”，应该是应用层概念，没有看到其他的于此相关的东西），所占区块的数目；attrib（不晓得是啥）；modified（同样不晓得是啥）；


```c++
  private:
    const File_system* fs_;
    Enttype     ftype;
    std::string fname_;
    uint64_t    block_;
    uint64_t    parent_; //< Parent's block#
    uint64_t    size_;
    uint32_t    attrib_;
    uint32_t    modif;
  }; //< struct Dirent
```
所有的成员都是私有，但都有对外的读接口

（modified接口有一行good luck注释，并不知道为啥要goodluck，可能并没有实现？）

作为一个文件，他的对外应用接口有 read(), ls(), stat(),device_id()（这个在一个底层类中定义，所以这里先不说）
## read

dirent 重载了4个 read() ， 
```c++
//fs就是const File_system* fs_;
  void Dirent::read(uint64_t pos, uint64_t n, on_read_func fn) {
    fs_->read(*this, pos, n, fn);
  }

  /** Read the whole file, async **/
  void Dirent::read(on_read_func fn) {
    read(0, size_, fn);
  }

  /** Read sync **/
  Buffer Dirent::read(uint64_t pos, uint64_t n) {
    return fs_->read(*this, pos, n);
    //这里的read也是通过on_read_func fn实现的

  }

  /** Read the whole file, sync, to string **/
  std::string Dirent::read() {
    return read(0, size_).to_string();
  }
```
显然，这里面除了 `on_read_func fn`外都很好理解。这个先不说，因为牵扯的蛮多的。。
## other
ls，stat 都是文件系统实现的，这里提供一个接口

# filesystem
File_system是一个结构体，里面没有任何成员，只有一些函数。

都是纯虚函数
这个结构体的存在的目的就是为了让具体的文件系统继承他
（比如FAT 就有`struct FAT : public File_system`）

## stat

这里的 stat（）函数，和写lab3时用的stat差不多
stat 定义了很多，分为async和sync。
最后都会调用一个纯虚函数 ，也就是说，所有的stat都要在写文件系统的时候实现
```c++
    /** Return information about a file or directory - async */
    virtual void stat(Path_ptr, on_stat_func fn, const Dirent* const = nullptr) const = 0;

    /** Stat async - for various types of path initializations **/
    template <typename P = Path>
    inline void stat(P pathstr, on_stat_func fn, const Dirent* const = nullptr) const;

    /** Return information about a file or directory relative to dirent - sync*/
    virtual Dirent stat(Path, const Dirent* const = nullptr) const = 0;

    /** Return information about a file or directory relative to dirent - sync*/
    template <typename P = Path>
    inline Dirent stat(P, const Dirent* const = nullptr) const;

```
template <typename P = Path> 指出模板 P 默认是 Path。 Path是他们已经实现好的一个类，而且我们可以直接拿来用。在 path.cpp|hpp 有详细的注释

`on_stat_func fn` 和之前的 `on_read_func fn` 一样。这里先给出人家的声明

  using on_read_func  = delegate<void(buffer_t)>;
  using on_write_func = delegate<void(bool error)>;

  这里using是别名的意思（类似define），delegate是一个讨厌的东西
```c++
//in delegate.hpp
template<
	typename R, typename... Args,
	template<size_t, size_t, typename, typename...> class Spec,//define in delegate.hpp
	size_t size,
	size_t align//define in delegate.hpp
>
class delegate<R(Args...), Spec, size, align>
```

delegate.hpp我一个人看不太懂，希望能有救兵帮我




##  read_file
显然，这里面除了 `on_read_func fn`外都很好理解。
关于 Buffer ， buffer是一个结构体，是一个经过包装的对外的结构体。
他包装自`os::mem::buf_ptr`，还没看

read 和 stat 一样，都要由具体的文件系统实现



## print_subtree
人如其名，已经封装好了


# FAT
就是FAT
可以选择 FAT12, FAT16, FAT32。

```c++
    // device we can read and write sectors to
    hw::Block_device& device;
```
他的对外接口有
```
/// ----------------------------------------------------- ///
    void init(uint64_t lba, uint64_t size, on_init_func on_init) override;

    int device_id() const noexcept override;

    // path is a path in the initialized filesystem
    void  ls     (const std::string& path, on_ls_func) const override;
    void  ls     (const Dirent& entry,     on_ls_func) const override;
    List  ls     (const std::string& path) const override;
    List  ls     (const Dirent&) const override;

    /** Read @n bytes from file pointed by @entry starting at position @pos */
    void   read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) const override;
    Buffer read(const Dirent&, uint64_t pos, uint64_t n) const override;

    // return information about a filesystem entity
    void   stat(Path_ptr, on_stat_func, const Dirent* const start) const override;
    Dirent stat(Path ent, const Dirent* const start) const override;
    // async cached stat
    void cstat(const std::string&, on_stat_func) override;

    // returns the name of the filesystem
    std::string name() const override

    uint64_t block_size() const noexcept override;
    /// ----------------------------------------------------- ///

    // constructor
    FAT(hw::Block_device& dev);
    virtual ~FAT() = default;
```
分别在 `fat.cpp` `fat_async.cpp` `fat_sync.cpp`中实习

## fat.cpp
### FAT(hw::Block_device& dev);
```cpp
  FAT::FAT(hw::Block_device& dev)
    : device(dev) {
    //
  }
```
给 private 成员 device 赋值 dev;
关于 Block_device 这是一个抽象基类，提供接口。一个简单的小说明
```c++
//in test.cpp
#include<stdio.h>
class A {
public:
    virtual void rua()=0;

};
class B : public A{
public:
    void rua() override {
        printf("rua\n");
    }
};

int main(void){

    B b;
    A* a = &b;
    a->rua();
}
```
输出结果
```bash
[tyl@rua test]$ ./a.out 
rua
```
可以简单，虽然 a 是基类 A 的指针，但可以访问 B 中的函数，因为A 中 rua() 被定义为纯虚函数

这里也一样。实例化 FAT 的时候，需要将一个实例化的，继承自 BLock_device 的类，这里是 Memdisk 类，传给 FAT()。这样做的好处是，文件系统与设备实现分离。

### void FAT::init(const void* );
先放代码
去掉了一些无用的信息

```c++
  void FAT::init(const void* base_sector) {

    // assume its the master boot record for now
    auto* mbr = (MBR::mbr*) base_sector;

    MBR::BPB* bpb = mbr->bpb();
    this->sector_size = bpb->bytes_per_sector;

    if (UNLIKELY(this->sector_size < 512)) {
      fprintf(stderr,
          "Invalid sector size (%u) for FAT32 partition\n", sector_size);
      fprintf(stderr,
          "Are you initializing the correct partition?\n");
      os::panic("FAT32: Invalid sector size");
    }

    // sector count
    if (bpb->small_sectors)
      this->sectors  = bpb->small_sectors;
    else
      this->sectors  = bpb->large_sectors;

    // sectors per FAT (not sure about the rule here)
    this->sectors_per_fat = bpb->sectors_per_fat;
    if (this->sectors_per_fat == 0)
      this->sectors_per_fat = *(uint32_t*) &mbr->boot[25];

    // root dir sectors from root entries
    this->root_dir_sectors = ((bpb->root_entries * 32) + (sector_size - 1)) / sector_size;

    // calculate index of first data sector
    this->data_index = bpb->reserved_sectors + (bpb->fa_tables * this->sectors_per_fat) + this->root_dir_sectors;

    // number of reserved sectors is needed constantly
    this->reserved = bpb->reserved_sectors;

    // number of sectors per cluster is important for calculating entry offsets
    this->sectors_per_cluster = bpb->sectors_per_cluster;

    // calculate number of data sectors
    this->data_sectors = this->sectors - this->data_index;

    // calculate total cluster count
    this->clusters = this->data_sectors / this->sectors_per_cluster;

    // now that we're here, we can determine the actual FAT type
    // using the official method:
    if (this->clusters < 4085) {
      this->fat_type = FAT::T_FAT12;
      this->root_cluster = 2;
    }
    else if (this->clusters < 65525) {
      this->fat_type = FAT::T_FAT16;
      this->root_cluster = 2;
    }
    else {
      this->fat_type = FAT::T_FAT32;
      this->root_cluster = *(uint32_t*) &mbr->boot[33];
      this->root_cluster = 2;
    }
  }
```

可以看到，初始化了一些信息
感觉没啥好说的



### void FAT::init(uint64_t , uint64_t , on_init_func);

### bool FAT::int_dirent(uint32_t, const void*, dirvector&) const;


## fat_async.cpp

## fat_sync.cpp
