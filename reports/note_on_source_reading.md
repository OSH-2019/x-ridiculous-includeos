# 源代码阅读 Q&A

## Understand
很有用。Report [见此](http://home.ustc.edu.cn/~jauntyliu/includeOS_understand_html/)

可以查看目录下的 C/C++ 函数的 Declaration 和 Define，以及 Called By 信息。

有些信息 Understand 找不到（比如一些 Define），请参见「在 Understand 找不到怎么办？」一节。

## Cytoscape
export INSTALL4J_JAVA_HOME=/usr/lib/jvm/java-8-openjdk/jre/
on Arch

可以看 Understand 生成的代码依赖图，不过并不是非常实用（太多了）

## 在 Understand 找不到怎么办？
可能是汇编引入的符号。

- https://stackoverflow.com/questions/4468361/search-all-of-git-history-for-a-string

- Github Search -> This Repository
