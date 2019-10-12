# NANOEnPer_V1.0
First release version for enviroment percetion which had built on JetsonNano

功能简介：通过摄像头实时采集的图像数据进行非机动车道和机动车道的判断，通过串口与滑板车身的喇叭通信，进行报警的提示。

需要搭配硬件环境，以下功能需要联系软件组更新

1.车载喇叭
  部分车上未装喇叭，需要安装喇叭进行串口通信

2.专用电源
  该电源不仅能提供滑板车电源并且与车身控制板连接，可以通过双按左右车把进行开启与关闭程序功能。

使用步骤
  
  通上电源，双按左右车把五秒看到仪表盘显示99，表示开启JetsonNano板的电源，等待1分钟左右听到“滴滴”连续三声，表示本程序启动。中途任意时刻，可以通过双按左右车把五秒，看到仪表盘显示88表示关闭了JetsonNano板的电源。

该文档中，Test为无效文件，可不予理会。

JetsonNano自启动脚本在～/test_auto_start文件中，Ubuntu18.04自启动的方法参见：https://www.cnblogs.com/it-tsz/p/9852301.html
