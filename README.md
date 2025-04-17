# wio-terminal-WIFI-clock (基于Seeed的wio terminal)
# wio terminal is developed as a networked real-time clock

在arduino上安装一些库就可以开始
	启动 Arduino 应用程序
	双击您之前下载的 Arduino IDE 应用程序
 	打开 LED 闪烁示例草图：文件 > 示例 >01.Basics > Blink
	![image](https://github.com/user-attachments/assets/3a3c27a4-feee-4563-b462-cf403e94b6ad)
	打开您的 Arduino IDE，单击 File > Preferences，然后将以下 url 复制到 Additional Boards Manager URLs：
	![image](https://github.com/user-attachments/assets/581e803f-75c5-46d3-897f-3be6862bb984)
	单击 Board Manager > Board Manager > Tools > 然后在 Boards Manager 中搜索 Wio Terminal
 	![image](https://github.com/user-attachments/assets/0bdb6115-d640-4d6a-8f1a-52165244f28a)
	您需要在 Tools > Board 菜单中选择与您的 Arduino 相对应的条目。 选择 Wio 终端
 	![image](https://github.com/user-attachments/assets/246716bd-c063-4c8a-aff0-2ce4e28cb585)
	从 Tools -> Port 菜单中选择 Wio Terminal 板的串行设备。这可能是 COM3 或更高版本（COM1 和 COM2 通常保留用于硬件串行端口）。要找出答案，您可以断开 Wio Terminal 板并重新打开菜单;消失的条目应该是 Arduino 板。重新连接板并选择该串行端口
 ![image](https://github.com/user-attachments/assets/f13300aa-be71-4918-a15a-787d62bbad56)
	现在，只需单击环境中的 Upload 按钮。等待几秒钟，如果上传成功，状态栏中将显示消息“Done uploading.”
![image](https://github.com/user-attachments/assets/9a0a90c5-4b8b-4524-b926-5a39ec64b058)
上传完成后几秒钟，您应该会看到 Wio 终端底部的 LED 开始闪烁。如果是这样，恭喜！您已经启动并运行了 Wio Terminal
如果在使用的使用出现了错误，建议您去看https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/#faq

接下来继续安装WIFI相关的库
![image](https://github.com/user-attachments/assets/33dba935-a38a-4653-9f27-7f4ab861da3e)
为了您的方便，我们已将所有库汇总在一起。因此，对于未来的文档和更新当前的文档，你只需要安装 rpcwifi，然后它就会为你安装所有依赖 wifi 的库

![image](https://github.com/user-attachments/assets/d4b9841f-0050-4e4d-bff8-36291e2de87d)
当您完成了以上的所有步骤，请将代码上传！

你将看到
![634ccff745319dfe0ede098774c1342f_compress](https://github.com/user-attachments/assets/30a4966c-c1ef-4adb-b6e6-ca531dc97fb5)
那么恭喜你完成了这个项目！
