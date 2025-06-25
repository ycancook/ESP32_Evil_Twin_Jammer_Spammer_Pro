# ESP32_Evil_Twin_Jammer_Spammer_Pro
# Web Flash: https://esp.huhn.me/
## Offset 	-> 	Bin
### 0x1000 	-> 	ESP32_Evil_Twin_Pro.ino.bootloader
### 0x8000 	-> 	ESP32_Evil_Twin_Pro.ino.partitions
### 0x10000 -> 	ESP32_Evil_Twin_Pro.ino
# * See the Readme.html file to connect to other modules

<h2>Default Settings</h2>
    <table>
        <tr>
            <th>Setting</th>
            <th>Value</th>
        </tr>
        <tr>
            <td>IP Address</td>
            <td>172.0.0.1</td>
        </tr>
        <tr>
            <td>Access point SSID</td>
            <td>CHOMTV</td>
        </tr>
        <tr>
            <td>Password</td>
            <td>@@@@2222</td>
        </tr>
        <tr>
            <td>Review the password log saved at the Evil-Twin site</td>
            <td>http://172.0.0.1/'Password' (default @@@@2222)</td>
        </tr>
        <tr>
            <td>Exit Evil-Twin Page</td>
            <td>At the Evil-Twin Page Enter 'Password' in the password input field</td>
        </tr>
    </table>
<h2>Jump Pin Connections for ESP32</h2>
    <table>
        <tr>
            <th>Configuration</th>
            <th>Connections</th>
        </tr>
        <tr>
            <td>ESP32 = RTL8720</td>
            <td>
                GND = GND<br>
                Vin = 5V<br>
                D34(GPIO34) = PB1<br>
                D35(GPIO35) = PB2<br>
                D32(GPIO32) = PA30<br>
                D25(GPIO25) = PA25<br>
                D26(GPIO26) = PA26
            </td>
        </tr>
</table>

<h2>Jump Pin Connections for ESP32 vs RF24</h2>
    <table>
        <tr>
            <th>Interface</th>
            <th>Connections</th>
        </tr>
        <tr>
            <td>HSPI</td>
            <td>SCK = 14, MISO = 12, MOSI = 13, CS = 15, CE = 16</td>
        </tr>
        <tr>
            <td>VSPI</td>
            <td>SCK = 18, MISO = 19, MOSI = 23, CS = 21, CE = 22</td>
        </tr>
</table>

<h2>Hand Reset User Password</h2>
    <table>
        <tr>
            <th>Board</th>
            <th>Reset Procedure</th>
        </tr>
        <tr>
            <td>RTL8720</td>
            <td>Hold BURN Button 10s (GND + PA7)</td>
        </tr>
        <tr>
            <td>ESP32</td>
            <td>Hold BOOT Button 10s (GND + GPIO0)</td>
        </tr>
</table>
    

<a href="https://www.buymeacoffee.com/h3r2015p" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>

📜 DISCLAIMER

    ⚠️ This project is created for educational and ethical testing purposes only.

        The author is not responsible for any misuse or damage caused by this code.

        Do not use this software on networks you do not own or have explicit permission to test.

        Using this tool for unauthorized access or attacks may be illegal and punishable by law in your country.

        By using or distributing this code, you agree to use it only for ethical and legal purposes, such as penetration testing with proper authorization or learning about network security.

    👉 If you are unsure about the legality of using this code in your region, do not use it.

    ⚠️ Dự án này được tạo ra với mục đích nghiên cứu và giáo dục về an toàn thông tin.

       Tác giả không chịu trách nhiệm với bất kỳ hành vi lạm dụng hoặc thiệt hại nào do mã nguồn gây ra.

       Không sử dụng phần mềm này trên các mạng mà bạn không sở hữu hoặc không được cấp quyền rõ ràng.

       Việc sử dụng mã này để tấn công trái phép có thể vi phạm pháp luật và bị xử lý hình sự tại quốc gia của bạn.

       Khi sử dụng mã nguồn này, bạn đồng ý chỉ dùng nó cho mục đích hợp pháp, ví dụ như kiểm thử bảo mật có sự cho phép hoặc học tập.

    👉 Nếu không chắc chắn về tính pháp lý tại nơi bạn sinh sống, xin đừng sử dụng.
