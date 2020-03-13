# FTP服务器

- 这是由之前的简单的select+多进程版本改进得来的：https://github.com/NewBeeXX/ftpserver

- 此版本采用epoll+多线程实现的IO多路复用模型，单进程。

<img src="https://github.com/NewBeeXX/FTPserver-epoll-multiThread/blob/master/pic/struct.png" width = "500" div align=center />

[//]: ![图片](https://github.com/NewBeeXX/FTPserver-epoll-multiThread/blob/master/pic/struct.png)

- 对于请求文件目录、打印当前路径等耗时小的请求，直接在epoll的主循环中做。对于传输文件等耗时长的请求，另启动线程处理，文件读写加记录锁。

- 暂时只实现了被动模式的数据连接，FTP协议中普通的pwd、cd、ls、get、put命令可以正常执行，其余功能暂未完善。 

- 如何记录每个已登陆用户的当前工作目录？

<img src="https://github.com/NewBeeXX/FTPserver-epoll-multiThread/blob/master/pic/session.png" width = "200" div align=center />

[//]: ![图片](https://github.com/NewBeeXX/FTPserver-epoll-multiThread/blob/master/pic/session.png)

&ensp;&ensp;&ensp;&ensp;每个会话用结构体保存必要的信息，其中包括了表示用户当前目录的文件描述符，打印目录、上传/下载文件等功能调用内核提供的openat()函数即可使用相对路径，从而解决了单进程无法保存每个已登陆用户的工作目录的问题。

- epoll模型下如何剔除长时间无请求的连接？

&ensp;&ensp;&ensp;&ensp;ftp服务器为每个用户保留连接一段时间，超时需要自动关闭连接。每个连接的剩余时间不同，单纯的epoll利用超时参数不能够很好的解决这个问题。

&ensp;&ensp;&ensp;&ensp;这里采用了类似LRU页置换算法的思想。
维护一个双向链表，如下：

<img src="https://github.com/NewBeeXX/FTPserver-epoll-multiThread/blob/master/pic/list.png" width = "800" div align=center />

[//]: ![图片](https://github.com/NewBeeXX/FTPserver-epoll-multiThread/blob/master/pic/list.png)

&ensp;&ensp;&ensp;&ensp;每当新连接到来，生成一个会话，置剩余时间为最大剩余时间，放置于MRU端。

&ensp;&ensp;&ensp;&ensp;当连接再次发来请求时，利用哈希表(文件描述符到会话结构的映射)找到对应会话，取下并再次置于MRU端，重置剩余时间。

&ensp;&ensp;&ensp;&ensp;每次调用epoll_wait(),设置epoll的超时时间为LRU端会话的剩余时间，若epoll超时未监听到触发的事件，在LRU端检测去除超时的会话。

&ensp;&ensp;&ensp;&ensp;该算法很好的利用了从MRU端到LRU端会话剩余时间依次递减的性质，并且维护此双向链表的时间复杂度不大，每次请求到来将会话置于MRU端时间复杂度为O(1).



