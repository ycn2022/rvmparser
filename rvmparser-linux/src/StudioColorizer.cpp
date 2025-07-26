#include <cassert>
#include "Store.h"
#include "StudioColorizer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>  // 用于 strtol 
#include <cctype>   // 用于十六进制验证 
#include <vector>
#include <iomanip>  // 用于输出格式 
#include <algorithm>

#include "windows.h"
#include <filesystem>
#include <minwindef.h>
#include <libloaderapi.h>

namespace fs = std::filesystem;

StudioColorizer::StudioColorizer(Logger logger, const char* colorAttribute) :Colorizer(logger, colorAttribute)
{
}


// 转大写 
std::string to_upper(const std::string& s) {
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}

// 转小写 
std::string to_lower(const std::string& s) {
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

// 新增：字符串 trim 函数
std::string trim(const std::string& str) {
	// 查找第一个非空白字符位置
	auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
		return std::isspace(ch);
		});
	// 查找最后一个非空白字符位置 
	auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
		return std::isspace(ch);
		}).base();
	return (start < end) ? std::string(start, end) : "";
}

std::filesystem::path GetExecutablePath() {
#ifdef _WIN32
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
#else
	char buffer[PATH_MAX];
	readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif
	return std::filesystem::path(buffer).parent_path(); // 直接获取目录
}

void StudioColorizer::init(Store& store)
{
	Colorizer::init(store);

	fs::path exe_dir = GetExecutablePath(); // 假设返回类型是fs::path 
	fs::path full_path = exe_dir / "rvmcolorindex.txt";  // 使用operator/自动处理分隔符
	//auto rvmcolorindex = GetExecutablePath().append("rvmcolorindex.txt");

	std::ifstream file(full_path);
	if (!file.is_open()) {
		std::cerr << "错误：无法打开文件" << "rvmcolorindex.txt" << std::endl;
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		// 2. 解析逗号分隔的三个字段 
		std::istringstream iss(line);
		std::string indexStr, colorName, hexCode;

		if (!std::getline(iss, indexStr, ',') ||
			!std::getline(iss, colorName, ',') ||
			!std::getline(iss, hexCode))
		{
			std::cerr << "格式错误: " << line << std::endl;
			continue;
		}

		// 新增：TRIM 处理（核心改进）[8]()[9]()
		indexStr = trim(indexStr);
		colorName = trim(colorName);
		hexCode = trim(hexCode);


		// 3. 转换索引为整数 
		int index;
		try {
			index = std::stoi(indexStr);
		}
		catch (...) {
			std::cerr << "无效索引: " << indexStr << std::endl;
			continue;
		}

		// 4. 验证并转换十六进制RGB值 
		// 检查格式是否以#开头且长度为7 
		if (hexCode.empty() || hexCode[0] != '#' || hexCode.size() != 7) {
			std::cerr << "无效HEX格式: " << hexCode << std::endl;
			continue;
		}

		// 提取#后面的部分（6位十六进制）
		std::string hexDigits = hexCode.substr(1);
		char* endPtr;
		uint64_t rgb = std::strtoul(hexDigits.c_str(), &endPtr, 16);

		// 验证转换结果 
		if (endPtr != hexDigits.c_str() + hexDigits.size()) {
			std::cerr << "HEX转换失败: " << hexDigits << std::endl;
			continue;
		}

		// 5. 存储解析结果 
		std::string colorNameUpper = to_upper(colorName);
		std::string colorNameLower = to_lower(colorName);
		colorNameByMaterialId.insert(index, uint64_t(store.strings.intern(colorName.c_str())));
		colorByName.insert(uint64_t(store.strings.intern(colorNameUpper.c_str())), rgb);
		colorByName.insert(uint64_t(store.strings.intern(colorNameLower.c_str())), rgb);
		colorByName.insert(uint64_t(store.strings.intern(colorName.c_str())), rgb);
	}



	/*colorNameByMaterialId.insert(1, uint64_t(store.strings.intern("Gray")));
	colorNameByMaterialId.insert(2, uint64_t(store.strings.intern("Red")));
	colorNameByMaterialId.insert(3, uint64_t(store.strings.intern("Orange")));
	colorNameByMaterialId.insert(4, uint64_t(store.strings.intern("Yellow")));
	colorNameByMaterialId.insert(5, uint64_t(store.strings.intern("Green")));
	colorNameByMaterialId.insert(6, uint64_t(store.strings.intern("Cyan")));
	colorNameByMaterialId.insert(7, uint64_t(store.strings.intern("Blue")));
	colorNameByMaterialId.insert(8, uint64_t(store.strings.intern("Violet")));
	colorNameByMaterialId.insert(9, uint64_t(store.strings.intern("Brown")));
	colorNameByMaterialId.insert(10, uint64_t(store.strings.intern("White")));
	colorNameByMaterialId.insert(11, uint64_t(store.strings.intern("Pink")));
	colorNameByMaterialId.insert(12, uint64_t(store.strings.intern("Mauve")));
	colorNameByMaterialId.insert(13, uint64_t(store.strings.intern("Turquoise")));
	colorNameByMaterialId.insert(14, uint64_t(store.strings.intern("Indigo")));
	colorNameByMaterialId.insert(15, uint64_t(store.strings.intern("Black")));
	colorNameByMaterialId.insert(16, uint64_t(store.strings.intern("Magenta")));
	colorNameByMaterialId.insert(17, uint64_t(store.strings.intern("Whitesmoke")));
	colorNameByMaterialId.insert(18, uint64_t(store.strings.intern("Ivory")));
	colorNameByMaterialId.insert(19, uint64_t(store.strings.intern("Lightgrey")));
	colorNameByMaterialId.insert(20, uint64_t(store.strings.intern("Darkgrey")));
	colorNameByMaterialId.insert(21, uint64_t(store.strings.intern("Darkslate")));
	colorNameByMaterialId.insert(22, uint64_t(store.strings.intern("Brightred")));
	colorNameByMaterialId.insert(23, uint64_t(store.strings.intern("Coralred")));
	colorNameByMaterialId.insert(24, uint64_t(store.strings.intern("Tomato")));
	colorNameByMaterialId.insert(25, uint64_t(store.strings.intern("Plum")));
	colorNameByMaterialId.insert(26, uint64_t(store.strings.intern("Deeppink")));
	colorNameByMaterialId.insert(27, uint64_t(store.strings.intern("Salmon")));
	colorNameByMaterialId.insert(28, uint64_t(store.strings.intern("Brightorange")));
	colorNameByMaterialId.insert(29, uint64_t(store.strings.intern("Orangered")));
	colorNameByMaterialId.insert(30, uint64_t(store.strings.intern("Maroon")));
	colorNameByMaterialId.insert(31, uint64_t(store.strings.intern("Gold")));
	colorNameByMaterialId.insert(32, uint64_t(store.strings.intern("Lightyellow")));
	colorNameByMaterialId.insert(33, uint64_t(store.strings.intern("Lightgold")));
	colorNameByMaterialId.insert(34, uint64_t(store.strings.intern("Yellowgreen")));
	colorNameByMaterialId.insert(35, uint64_t(store.strings.intern("Springgreen")));
	colorNameByMaterialId.insert(36, uint64_t(store.strings.intern("Forestgreen")));
	colorNameByMaterialId.insert(37, uint64_t(store.strings.intern("Tomato")));
	colorNameByMaterialId.insert(38, uint64_t(store.strings.intern("Aquamarine")));
	colorNameByMaterialId.insert(39, uint64_t(store.strings.intern("Royalblue")));
	colorNameByMaterialId.insert(40, uint64_t(store.strings.intern("Navyblue")));
	colorNameByMaterialId.insert(41, uint64_t(store.strings.intern("Powderblue ")));
	colorNameByMaterialId.insert(42, uint64_t(store.strings.intern("Midnight")));
	colorNameByMaterialId.insert(43, uint64_t(store.strings.intern("Steelblue")));
	colorNameByMaterialId.insert(44, uint64_t(store.strings.intern("Beige")));
	colorNameByMaterialId.insert(45, uint64_t(store.strings.intern("Wheat")));
	colorNameByMaterialId.insert(46, uint64_t(store.strings.intern("Tan")));
	colorNameByMaterialId.insert(47, uint64_t(store.strings.intern("Orangered")));
	colorNameByMaterialId.insert(48, uint64_t(store.strings.intern("Khaki")));
	colorNameByMaterialId.insert(49, uint64_t(store.strings.intern("Chocolate")));
	colorNameByMaterialId.insert(50, uint64_t(store.strings.intern("Darkbrown")));
	colorByName.insert(uint64_t(store.strings.intern("GRAY")), 0x808080);
	colorByName.insert(uint64_t(store.strings.intern("RED")), 0xFF0000);
	colorByName.insert(uint64_t(store.strings.intern("ORANGE")), 0xFFA500);
	colorByName.insert(uint64_t(store.strings.intern("YELLOW")), 0xFFFF00);
	colorByName.insert(uint64_t(store.strings.intern("GREEN")), 0x008000);
	colorByName.insert(uint64_t(store.strings.intern("CYAN")), 0x00FFFF);
	colorByName.insert(uint64_t(store.strings.intern("BLUE")), 0x0000FF);
	colorByName.insert(uint64_t(store.strings.intern("VIOLET")), 0x8B00FF);
	colorByName.insert(uint64_t(store.strings.intern("BROWN")), 0xA52A2A);
	colorByName.insert(uint64_t(store.strings.intern("WHITE")), 0xFFFFFF);
	colorByName.insert(uint64_t(store.strings.intern("PINK")), 0xFFC0CB);
	colorByName.insert(uint64_t(store.strings.intern("MAUVE")), 0x6640FF);
	colorByName.insert(uint64_t(store.strings.intern("TURQUOISE")), 0x30D5C8);
	colorByName.insert(uint64_t(store.strings.intern("INDIGO")), 0x4B0080);
	colorByName.insert(uint64_t(store.strings.intern("BLACK")), 0x000000);
	colorByName.insert(uint64_t(store.strings.intern("MAGENTA")), 0xFF00FF);
	colorByName.insert(uint64_t(store.strings.intern("WHITESMOKE")), 0xF5F5F5);
	colorByName.insert(uint64_t(store.strings.intern("IVORY")), 0xFFFFF0);
	colorByName.insert(uint64_t(store.strings.intern("LIGHTGREY")), 0xD3D3D3);
	colorByName.insert(uint64_t(store.strings.intern("DARKGREY")), 0xA9A9A9);
	colorByName.insert(uint64_t(store.strings.intern("DARKSLATE")), 0x323A3A);
	colorByName.insert(uint64_t(store.strings.intern("BRIGHTRED")), 0xf93822);
	colorByName.insert(uint64_t(store.strings.intern("CORALRED")), 0xFF7F50);
	colorByName.insert(uint64_t(store.strings.intern("TOMATO")), 0xFF6347);
	colorByName.insert(uint64_t(store.strings.intern("PLUM")), 0x8E4585);
	colorByName.insert(uint64_t(store.strings.intern("DEEPPINK")), 0xFF1493);
	colorByName.insert(uint64_t(store.strings.intern("SALMON")), 0xFFA07A);
	colorByName.insert(uint64_t(store.strings.intern("BRIGHTORANGE")), 0xFFA500);
	colorByName.insert(uint64_t(store.strings.intern("ORANGERED")), 0xFF4500);
	colorByName.insert(uint64_t(store.strings.intern("MAROON")), 0x800000);
	colorByName.insert(uint64_t(store.strings.intern("GOLD")), 0xFFD700);
	colorByName.insert(uint64_t(store.strings.intern("LIGHTYELLOW")), 0xFFFFE0);
	colorByName.insert(uint64_t(store.strings.intern("LIGHTGOLD")), 0xFFD700);
	colorByName.insert(uint64_t(store.strings.intern("YELLOWGREEN")), 0x9ACD32);
	colorByName.insert(uint64_t(store.strings.intern("SPRINGGREEN")), 0x00FF80);
	colorByName.insert(uint64_t(store.strings.intern("FORESTGREEN")), 0x228B22);
	colorByName.insert(uint64_t(store.strings.intern("TOMATO")), 0xFF6347);
	colorByName.insert(uint64_t(store.strings.intern("AQUAMARINE")), 0x7FFFD4);
	colorByName.insert(uint64_t(store.strings.intern("ROYALBLUE")), 0x00206A);
	colorByName.insert(uint64_t(store.strings.intern("NAVYBLUE")), 0x000080);
	colorByName.insert(uint64_t(store.strings.intern("POWDERBLUE ")), 0x003399);
	colorByName.insert(uint64_t(store.strings.intern("MIDNIGHT")), 0x191970);
	colorByName.insert(uint64_t(store.strings.intern("STEELBLUE")), 0x4682B4);
	colorByName.insert(uint64_t(store.strings.intern("BEIGE")), 0xF5F5DC);
	colorByName.insert(uint64_t(store.strings.intern("WHEAT")), 0xF5DEB3);
	colorByName.insert(uint64_t(store.strings.intern("TAN")), 0xD2B48C);
	colorByName.insert(uint64_t(store.strings.intern("ORANGERED")), 0xFF4500);
	colorByName.insert(uint64_t(store.strings.intern("KHAKI")), 0xC3B091);
	colorByName.insert(uint64_t(store.strings.intern("CHOCOLATE")), 0xD2691E);
	colorByName.insert(uint64_t(store.strings.intern("DARKBROWN")), 0x8B4513);

	colorByName.insert(uint64_t(store.strings.intern("gray")), 0x808080);
	colorByName.insert(uint64_t(store.strings.intern("red")), 0xFF0000);
	colorByName.insert(uint64_t(store.strings.intern("orange")), 0xFFA500);
	colorByName.insert(uint64_t(store.strings.intern("yellow")), 0xFFFF00);
	colorByName.insert(uint64_t(store.strings.intern("green")), 0x008000);
	colorByName.insert(uint64_t(store.strings.intern("cyan")), 0x00FFFF);
	colorByName.insert(uint64_t(store.strings.intern("blue")), 0x0000FF);
	colorByName.insert(uint64_t(store.strings.intern("violet")), 0x8B00FF);
	colorByName.insert(uint64_t(store.strings.intern("brown")), 0xA52A2A);
	colorByName.insert(uint64_t(store.strings.intern("white")), 0xFFFFFF);
	colorByName.insert(uint64_t(store.strings.intern("pink")), 0xFFC0CB);
	colorByName.insert(uint64_t(store.strings.intern("mauve")), 0x6640FF);
	colorByName.insert(uint64_t(store.strings.intern("turquoise")), 0x30D5C8);
	colorByName.insert(uint64_t(store.strings.intern("indigo")), 0x4B0080);
	colorByName.insert(uint64_t(store.strings.intern("black")), 0x000000);
	colorByName.insert(uint64_t(store.strings.intern("magenta")), 0xFF00FF);
	colorByName.insert(uint64_t(store.strings.intern("whitesmoke")), 0xF5F5F5);
	colorByName.insert(uint64_t(store.strings.intern("ivory")), 0xFFFFF0);
	colorByName.insert(uint64_t(store.strings.intern("lightgrey")), 0xD3D3D3);
	colorByName.insert(uint64_t(store.strings.intern("darkgrey")), 0xA9A9A9);
	colorByName.insert(uint64_t(store.strings.intern("darkslate")), 0x323A3A);
	colorByName.insert(uint64_t(store.strings.intern("brightred")), 0xf93822);
	colorByName.insert(uint64_t(store.strings.intern("coralred")), 0xFF7F50);
	colorByName.insert(uint64_t(store.strings.intern("tomato")), 0xFF6347);
	colorByName.insert(uint64_t(store.strings.intern("plum")), 0x8E4585);
	colorByName.insert(uint64_t(store.strings.intern("deeppink")), 0xFF1493);
	colorByName.insert(uint64_t(store.strings.intern("salmon")), 0xFFA07A);
	colorByName.insert(uint64_t(store.strings.intern("brightorange")), 0xFFA500);
	colorByName.insert(uint64_t(store.strings.intern("orangered")), 0xFF4500);
	colorByName.insert(uint64_t(store.strings.intern("maroon")), 0x800000);
	colorByName.insert(uint64_t(store.strings.intern("gold")), 0xFFD700);
	colorByName.insert(uint64_t(store.strings.intern("lightyellow")), 0xFFFFE0);
	colorByName.insert(uint64_t(store.strings.intern("lightgold")), 0xFFD700);
	colorByName.insert(uint64_t(store.strings.intern("yellowgreen")), 0x9ACD32);
	colorByName.insert(uint64_t(store.strings.intern("springgreen")), 0x00FF80);
	colorByName.insert(uint64_t(store.strings.intern("forestgreen")), 0x228B22);
	colorByName.insert(uint64_t(store.strings.intern("tomato")), 0xFF6347);
	colorByName.insert(uint64_t(store.strings.intern("aquamarine")), 0x7FFFD4);
	colorByName.insert(uint64_t(store.strings.intern("royalblue")), 0x00206A);
	colorByName.insert(uint64_t(store.strings.intern("navyblue")), 0x000080);
	colorByName.insert(uint64_t(store.strings.intern("powderblue ")), 0x003399);
	colorByName.insert(uint64_t(store.strings.intern("midnight")), 0x191970);
	colorByName.insert(uint64_t(store.strings.intern("steelblue")), 0x4682B4);
	colorByName.insert(uint64_t(store.strings.intern("beige")), 0xF5F5DC);
	colorByName.insert(uint64_t(store.strings.intern("wheat")), 0xF5DEB3);
	colorByName.insert(uint64_t(store.strings.intern("tan")), 0xD2B48C);
	colorByName.insert(uint64_t(store.strings.intern("orangered")), 0xFF4500);
	colorByName.insert(uint64_t(store.strings.intern("khaki")), 0xC3B091);
	colorByName.insert(uint64_t(store.strings.intern("chocolate")), 0xD2691E);
	colorByName.insert(uint64_t(store.strings.intern("darkbrown")), 0x8B4513);*/


}
