这是一个nginx的自定义模块，用来依据ip所在区域进行过滤。

用法：

	1、下载代码到本地放到任意目录/xxx/yyy

	2、将/xxx/yyy/qqwry.dat拷贝到nginx根目录

	3、重新编译nginx configure命令加上选项：--add-module=/xxx/yyy

	4、make && make install

	5、编辑配置文件nginx.conf,该模块有两个指令可以使用：
		#开关控制命令： location_filter on|off；
		#白名单地区配置： location_filter_white_list "深圳 广州"

注意：本模块使用的ip数据库来自纯真网络cz88.net。
感谢他们提供的数据，同时经过测试，发现数据有不准确的情况。
所以建议生产环境购买更加精准的商业数据进行替换。

感谢这个同学对纯真ip的解析。
https://blog.csdn.net/iteye_15428/article/details/81622990
