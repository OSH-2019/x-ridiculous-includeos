首先看 start_loc，它是 const char * （因为 image_start_ 是 const char *）
所以在 fs::construct_buffer 处的函数调用会把模板实例化为 fs::buffer_t fs::construct_buffer<const char *&, const char *&>(const char *&args, const char *&args)​。

  template <typename... Args>
  buffer_t construct_buffer(Args&&... args) {
    return std::make_shared<os::mem::buffer> (std::forward<Args> (args)...);
  }

这里可以看到，其实就是调用了 std::make_shared 模板函数。 

std::make_shared<T> (Args &&... args) 的作用是用 Args && 调用构造函数来实例化一个 T 类型的对象。

所以当调用 fs::construct_buffer(start_loc, end_loc) 时，会大概变成

std::shared_ptr(new std::pmr::vector<uint8_t> (start_loc, end_loc))

这种感觉。总之就是 std::pmr::vector<uint8_t> (start_loc, end_loc) 这样初始化一个 vector。

这种初始化的做法可以参见下面 code ：

int arr[]={0,1,2,3,4,5};
vector<int> ivec(arr,arr+6);

具体的说，其实是用了这种构造函数：

template< class InputIt >
vector( InputIt first, InputIt last, 
        const Allocator& alloc = Allocator() );

其中 InputIt 是输入迭代器，而 vector 会从输入迭代到输出，并且把这些值作为 vector 的初始值。

因为 const char *start_loc 和 const char *end_loc 在这里满足输入迭代器的条件，所以可以这样使用。