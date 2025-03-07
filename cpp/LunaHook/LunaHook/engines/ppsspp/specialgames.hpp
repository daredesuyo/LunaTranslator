﻿#include <queue>
#include "emujitarg.hpp"
namespace ppsspp
{

	struct emfuncinfo
	{
		uint64_t type;
		int offset;
		int padding;
		decltype(HookParam::text_fun) hookfunc;
		decltype(HookParam::filter_fun) filterfun;
		const char *_id;
	};

}

namespace ppsspp
{
	void ULJS00403_filter(TextBuffer *buffer, HookParam *hp)
	{
		std::string result = buffer->strA();
		result = std::regex_replace(result, std::regex(R"((\\n)+)"), " ");
		result = std::regex_replace(result, std::regex(R"((\\d$|^\@[a-z]+|#.*?#|\$))"), "");
		buffer->from(result);
	}
	void ULJS00124_1(hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
	{
		auto a2 = (char *)PPSSPP::emu_arg(context)[1];
		auto len = *(BYTE *)(PPSSPP::emu_arg(context)[1] - 2);
		std::string collect;
		std::string hex;
		for (int i = 0; i < len; i++)
		{
			if (*(char *)(a2 + i))
				collect += *(char *)(a2 + i);
		}
		strReplace(collect, "\n", "");
		strReplace(collect, "\r", "");
		strReplace(collect, "\x01-", "");
		strReplace(collect, "\x01<", "");
		buffer->from(collect);
	}
	void ULJS00339(hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
	{
		auto a2 = PPSSPP::emu_arg(context)[0];

		auto vm = *(DWORD *)(a2 + (0x28));
		vm = *(DWORD *)PPSSPP::emu_addr(context, vm);
		vm = *(DWORD *)PPSSPP::emu_addr(context, vm + 8);
		uintptr_t address = PPSSPP::emu_addr(context, vm);
		auto len1 = *(DWORD *)(address + 4);
		auto p = address + 0x20;
		if (len1 > 4 && *(WORD *)(p + 2) == 0)
		{
			auto p1 = *(DWORD *)(address + 8);
			vm = *(DWORD *)PPSSPP::emu_addr(context, vm);
			vm = *(DWORD *)PPSSPP::emu_addr(context, vm + 0xC);
			p = PPSSPP::emu_addr(context, vm);
		}
		static int fm = 0;
		static std::string pre;
		auto b = fm;
		auto s = [](uintptr_t address)
		{
			auto frist = *(WORD *)address;
			auto lo = frist & 0xFF; // uppercase: 41->5A
			auto hi = frist >> 8;
			if (hi == 0 && (lo > 0x5a || lo < 0x41) /* T,W,? */)
			{
				return std::string();
			}
			std::string s;
			int i = 0;
			WORD c;
			char buf[3] = {0};
			while ((c = *(WORD *)(address + i)) != 0)
			{
				// reverse endian: ShiftJIS BE => LE
				buf[0] = c >> 8;
				buf[1] = c & 0xFF;

				if (c == 0x815e /* ／ */)
				{
					s += ' '; // single line
				}
				else if (buf[0] == 0)
				{
					//// UTF16 LE turned BE: 5700=>0057, 3100, 3500
					//// 4e00 6d00=>PLAYER
					// do nothing
					if (buf[1] == 0x4e)
					{
						s += "PLAYER";
						fm++;
					}
				}
				else
				{
					s += buf;
				}
				i += 2;
			}
			return s;
		}(p);
		if (b > 0)
		{
			fm--;
			return;
		}
		if (s == pre)
			return;
		pre = s;
		buffer->from(s);
	}

	void ULJM05019(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		auto _1 = s.find("\x81\xa5");
		auto _2 = s.find("\x81\xa1"); // ■ ▼
		auto _3 = min(_1, _2);
		if (_3 == s.npos)
			return;
		s = s.substr(0, _1);
		s = std::regex_replace(s, std::regex(R"(\x81m(.*?)\x81n)"), "");   // ［龍神］
		s = std::regex_replace(s, std::regex(R"(\x81o(.*?)\x81p)"), "$1"); // ｛龍の宝玉｝
		strReplace(s, "\n", "");
		strReplace(s, "\x81\x40", "");
		buffer->from(s);
	}
	void ULJS00124(TextBuffer *buffer, HookParam *hp)
	{
		CharFilter(buffer, '\n');
	}
	void NPJH50899(TextBuffer *buffer, HookParam *hp)
	{
		CharFilter(buffer, '\n');
		StringFilter(buffer, "\x81\x40", 2);
	}
	void ULJM06006(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "@L", 2);
		StringFilter(buffer, "@I", 2);
		StringFilter(buffer, "@P", 2);
	}
	void ULJM06147(TextBuffer *buffer, HookParam *hp)
	{
		CharFilter(buffer, L'R');
	}
	void ULJM05587_2(TextBuffer *buffer, HookParam *hp)
	{
		if (buffer->viewW()[0] == L'　' || buffer->viewW()[0] == L'「')
			return buffer->clear();
	}
	void ULJM05587_1(TextBuffer *buffer, HookParam *hp)
	{
		if (buffer->viewW()[0] != L'　' && buffer->viewW()[0] != L'「')
			return buffer->clear();
		CharFilter(buffer, L'　');
		CharFilter(buffer, L'\n');
	}
	void ULJM05770(TextBuffer *buffer, HookParam *hp)
	{
		std::string s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(\x81\x6f(.*?)\x81\x5e(.*?)\x81\x70)"), "$2"); // ｛みす／御簾｝
		buffer->from(s);
		CharFilter(buffer, '\n');
	}
	void NPJH50754(TextBuffer *buffer, HookParam *hp)
	{
		std::string s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(<R(.*?)>(.*?)</R>)"), "$2"); // ｛みす／御簾｝
		buffer->from(s);
	}
	void ULJM06378(TextBuffer *buffer, HookParam *hp)
	{
		auto ws = StringToWideString(buffer->viewA(), 932).value();
		strReplace(ws, L"\\", L"");
		strReplace(ws, L"$", L"");
		buffer->from(WideStringToString(ws, 932));
	}
	void NPJH50215(TextBuffer *buffer, HookParam *hp)
	{
		auto ws = StringToWideString(buffer->viewA(), 932).value();
		strReplace(ws, L"r", L"\n");
		if (ws.find(L"wc"))
		{
			ws = ws.substr(0, ws.find(L"wc"));
		}
		ws = std::regex_replace(ws, std::wregex(LR"(l(.*?)\((.*?)\))"), L"$1");
		buffer->from(WideStringToString(ws, 932));
	}

	void NPJH50909_filter(TextBuffer *buffer, HookParam *hp)
	{
		auto ws = StringToWideString(buffer->viewA(), 932).value();
		ws = std::regex_replace(ws, std::wregex(L"(\\%N)+"), L" ");
		ws = std::regex_replace(ws, std::wregex(L"\\%\\@\\%\\d+"), L"");
		std::wsmatch match;
		if (std::regex_search(ws, match, std::wregex(L"(^[^「]+)「")))
		{
			std::wstring name = match[1].str();
			ws = std::regex_replace(ws, std::wregex(L"^[^「]+"), L"");
			ws = name + L"\n" + ws;
		}
		buffer->from(WideStringToString(ws, 932));
	}
	void ULJM06119_filter(TextBuffer *buffer, HookParam *hp)
	{
		std::string s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(/\[[^\]]+./g)"), "");
		s = std::regex_replace(s, std::regex(R"(/\\k|\\x|%C|%B)"), "");
		s = std::regex_replace(s, std::regex(R"(/\%\d+\#[0-9a-fA-F]*\;)"), "");
		s = std::regex_replace(s, std::regex(R"(/\n+)"), " ");
		buffer->from(s);
	}
	void ULJM06036_filter(TextBuffer *buffer, HookParam *hp)
	{
		std::wstring result = buffer->strW();
		result = std::regex_replace(result, std::wregex(LR"(<R([^\/]+).([^>]+).>)"), L"$2");
		result = std::regex_replace(result, std::wregex(LR"(<[A-Z]+>)"), L"");
		buffer->from(result);
	}
	namespace Corda
	{
		std::string readBinaryString(uintptr_t address, bool *haveName)
		{
			*haveName = false;
			if ((*(WORD *)address & 0xF0FF) == 0x801b)
			{
				*haveName = true;
				address = address + 2; // (1)
			}
			std::string s;
			int i = 0;
			uint8_t c;
			if (*(BYTE *)(address + i) == 0xaa)
				i += 1;

			while (true)
			{
				c = *(uint8_t *)(address + i);
				if (!c)
				{
					if (*(uint8_t *)(address + i + 1) == 0xaa)
						i += 2;
					else
						break;
				}
				else if (c == 0x1b)
				{
					if (*haveName)
						return s; // (1) skip junk after name

					c = *(uint8_t *)(address + (i + 1));
					if (c == 0x7f)
						i += 5;
					else
						i += 2;
				}
				else if (c == 0x0a)
				{
					s += '\n';
					i += 1;
				}
				else if (c == 0x20)
				{
					s += ' ';
					i += 1;
				}
				else
				{
					auto len = 1 + (IsShiftjisLeadByte(*(BYTE *)(address + i)));
					s += std::string((char *)(address + i), len);
					i += len; // encoder.encode(c).byteLength;
				}
			}
			return s;
		}
	}
	void ULJM05428(hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
	{
		auto address = PPSSPP::emu_arg(context)[hp->offset];
		bool haveNamve;
		auto s = Corda::readBinaryString(address, &haveNamve);
		*split = haveNamve;
		buffer->from(s);
	}

	void ULJM06281(TextBuffer *buffer, HookParam *hp)
	{
		CharFilter(buffer, '\n');
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(@\d+r)"), "");
		buffer->from(s);
	}
	std::wstring ULJM06143Code(std::string s)
	{
		std::wstring ws;
		wchar_t hex[100];
		for (int i = 0; i < s.size() - 1; i += 2)
		{
			// 这个游戏用了一个不知道什么规律的傻逼编码，规律很乱，就这样吧，懒得弄了。
			auto _this = _byteswap_ushort(*(wchar_t *)(s.c_str() + i));
			swprintf(hex, L"%04x", _this);
			if (0) //_this < 0x8200)
			{
				auto h = _byteswap_ushort(_this + 0x11e);
				ws += StringToWideString(std::string((char *)&h, 2), 932).value_or(std::wstring(L"[") + hex + L"]");
			}
			else
			{
				ws += std::wstring(L"[") + hex + L"]";
			}
		}
		return ws;
	}
	void ULJM06143_1(hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
	{
		auto cs = (char *)PPSSPP::emu_arg(context)[2];

		while (*(WORD *)(cs - 1))
			cs -= 1;
		cs += 1;
		std::wstring ws;
		while (*(char *)cs)
		{
			std::string s = cs;
			if (!IsDBCSLeadByteEx(932, s[0]))
				ws += StringToWideString(cs, 932).value_or(L"") + L" ";
			else
				ws += ULJM06143Code(s);
			cs += s.size() + 1;
		}
		buffer->from(ws);
	}
	void ULJM06143(hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
	{
		std::string s = (char *)PPSSPP::emu_arg(context)[1];
		buffer->from(ULJM06143Code(s));
	}
	void NPJH50902(hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
	{
		auto cs = (char *)PPSSPP::emu_arg(context)[3];
		while (*(WORD *)(cs - 1))
			cs -= 1;
		cs += 1;
		std::string ws;
		while (*(char *)cs)
		{
			std::string s = cs;
			for (int i = 0; i < s.size();)
			{
				if ((BYTE)s[i] == 0x87)
				{
					if (((BYTE)s[i + 1] == 0x86) || (BYTE)s[i + 1] == 0x85)
						ws += "\x81\x5b";
					i += 2;
				}
				else
				{
					ws += s[i];
					i += 1;
				}
			}
			cs += s.size() + 1;
		}
		strReplace(ws, "\n", "");
		buffer->from(ws);
	}
	void ULJM06220(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		s = s.substr(s.find("#n"));
		strReplace(s, "#n", "");
		s = std::regex_replace(s, std::regex(R"((#[A-Za-z]+\[(\d*[.,])?\d+\])+)"), "");
		buffer->from(s);
	}
	void NPJH50831_1(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		if (s.find("#n\x81\x75") == s.npos)
			return buffer->clear();
		buffer->from(s.substr(0, s.find("#n\x81\x75")));
	}
	void ULJM06266(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		buffer->from(s.substr(0, s.find("#n")));
	}
	void NPJH50831(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "#n", 2);
	}
	void ULJM05943F(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "#n", 2);
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"((#[A-Za-z]+\[(\d*[.,])?\d+\])+)"), "");
		buffer->from(s);
	}
	void ULJM05867_1(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		if (s.find("#n") == s.npos)
			return buffer->clear();
		ULJM05943F(buffer, hp);
	}
	void ULJM05867_2(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		if (s.find("#n") != s.npos)
			return buffer->clear();
		ULJM05943F(buffer, hp);
	}
	void ULJM05276(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "@y", 2);
		StringFilter(buffer, "@w", 2);
		StringFilter(buffer, "\\c", 2);
		StringFilter(buffer, "\\n", 2);
	}
	void ULJM06289(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "\x81\x40", 2);
		ULJM05943F(buffer, hp);
	}
	void ULJM06167(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		if (s == "0")
			return buffer->clear();
		ULJM05943F(buffer, hp);
	}
	void ULJM05610(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "#cr0", 4);
		StringFilter(buffer, "#wa1", 4);
		StringFilter(buffer, "#wa0", 4);
	}

	void ULJM05565(TextBuffer *buffer, HookParam *)
	{
		StringFilter(buffer, "/K", 2);
		StringCharReplacer(buffer, "\x81\x9b", 2, '\n');
		if (buffer->size == 0)
			return;
		static std::string last, lastx;
		auto s = buffer->strA();
		if (startWith(s, "sc"))
			return buffer->clear();
		if (endWith(last, s))
			return buffer->clear();
		last = s;
		if (lastx == last)
			return buffer->clear();
		lastx = last;
	}
	void NPJH50901(TextBuffer *buffer, HookParam *)
	{
		static std::string last;
		static lru_cache<std::string> lastx(10); // 颜色&注解会把句子分开
		auto s = buffer->strA();
		if (endWith(last, s))
			return buffer->clear();
		last = s;
		if (lastx.touch(last))
			return buffer->clear();

		std::string x;
		for (int i = 0; i < s.size();)
		{

			if (s[i] == 0x1b)
			{
				if ((BYTE)s[i + 1] == 0xb5)
					i += 2;
				else if ((BYTE)s[i + 1] == 0xb4 || (BYTE)s[i + 1] == 0xb6)
					i += 3;
				else
					i += 1;
			}
			else
			{
				x += s[i];
				i++;
			}
		}

		strReplace(x, "\n", "");
		strReplace(x, "\x81\x40", "");
		buffer->from(x);
	}
	void FULJM05603(TextBuffer *buffer, HookParam *)
	{
		StringFilter(buffer, "%N", 2);
		StringFilter(buffer, "%K", 2);
		StringFilter(buffer, "%P", 2);
		StringFilter(buffer, "%V", 2);
		StringFilter(buffer, "%LC", 3);
		StringFilter(buffer, "%LE", 3);
		StringFilter(buffer, "%FS", 3);
		StringFilter(buffer, "%FE", 3);
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(\{(.*?)\}\[(.*?)\])"), "$1");
		s = std::regex_replace(s, std::regex(R"(%O\d{3})"), "$1");
		s = std::regex_replace(s, std::regex(R"(%S\d{3})"), "$1");
		buffer->from(s);
	}
	void ULJM05821(TextBuffer *buffer, HookParam *hp)
	{
		static lru_cache<std::string> lastx(2);
		auto s = buffer->strA();
		if (lastx.touch(s))
			return buffer->clear();
		FULJM05603(buffer, hp);
	}

	void ULJM05810(hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
	{
		auto data = PPSSPP::emu_arg(context)[0x0f];
		data = data + 400;
		std::string s;
		while (true)
		{
			std::string sub = (char *)data;
			s += sub;
			data += sub.size() + 1;
			if (!*(char *)data)
				break;
		}
		strReplace(s, "\n", "");
		buffer->from(s);
	}
	namespace NPJH50530
	{
		std::string current;
		void T(TextBuffer *buffer, HookParam *)
		{
			current = buffer->strA();
			StringCharReplacer(buffer, "\\n", 2, '\n');
		}
		void N(TextBuffer *buffer, HookParam *)
		{
			auto current1 = buffer->strA();
			if (current == current1)
				buffer->clear();
			else
			{
				StringCharReplacer(buffer, "\\n", 2, '\n');
			}
		}
	}
	void FNPJH50243(TextBuffer *buffer, HookParam *)
	{
		auto s = buffer->strW();
		s = std::regex_replace(s, std::wregex(LR"(<(.*?)\|(.*?)>)"), L"$1");
		buffer->from(s);
	}
	void FNPJH50459(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(#SCL\((.*?)\)(.*?)#ECL)"), "$2");
		strReplace(s, "\n\r\n", "\n");
		buffer->from(s);
	}
	void ULJM05657(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "@n", 2);
		auto s = buffer->strA();
		if (endWith(s, "\x81\x76"))
			s = "\x81\x75" + s;
		buffer->from(s);
	}
	void ULJS00579(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "\\n", 2);
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(@\w(.*?)@\d)"), "$1");
		buffer->from(s);
	}
	void FNPJH50127(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "\\n", 2);
	}
	void ULJM05756(TextBuffer *buffer, HookParam *hp)
	{
		static std::string last;
		auto s = buffer->strA();
		if (s == last)
		{
			last = s;
			return buffer->clear();
		}
		last = s;
		strReplace(s, "<D>", u8"ー");
		buffer->from(s);
	}
	void NPJH50224(TextBuffer *buffer, HookParam *hp)
	{
		CharFilter(buffer, '\n');
		StringCharReplacer(buffer, "*p", 2, '\n');
	}
	void NPJH50535(TextBuffer *buffer, HookParam *hp)
	{
		static std::string last;
		auto s = buffer->strA();
		if (s == last)
		{
			last = s;
			return buffer->clear();
		}
		last = s;
		s = std::regex_replace(s, std::regex(R"(@\d{2})"), "");
		buffer->from(s);
	}
	void ULJS00354(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		static lru_cache<std::string> last(2);
		if (last.touch(s))
			return buffer->clear();
		strReplace(s, "#n", "");
		s = std::regex_replace(s, std::regex(R"(#R(.*?)\((.*?)\))"), "$1");
		buffer->from(s);
	}
	void ULJM05458(TextBuffer *buffer, HookParam *hp)
	{
		static int i = 0;
		if ((i++ % 2) || all_ascii((char *)buffer->buff, buffer->size))
			return buffer->clear();
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(\[(.*?)\*(.*?)\])"), "$1");
		buffer->from(s);
	}
	void ULJM05691(TextBuffer *buffer, HookParam *hp)
	{
		auto ws = StringToWideString(buffer->viewA(), 932).value();
		if (startWith(ws, L"disc"))
			return buffer->clear();
		strReplace(ws, L"\\", L"");
		strReplace(ws, L"$", L"");
		strReplace(ws, L"%　&", L"");
		buffer->from(WideStringToString(ws, 932));
	}
	void ULJM05796(TextBuffer *buffer, HookParam *hp)
	{
		static std::wstring last;
		auto ws = StringToWideString(buffer->viewA(), 932).value();
		strReplace(ws, L"\\", L"");
		if (last == ws)
		{
			buffer->clear();
			last = ws;
		}
		else
		{
			if (endWith(last, ws))
				buffer->clear();
			else
			{
				buffer->from(WideStringToString(ws, 932));
				last = ws;
			}
		}
	}
	void NPJH50900(TextBuffer *buffer, HookParam *hp)
	{
		auto ws = StringToWideString(buffer->viewA(), 932).value();
		ws = std::regex_replace(ws, std::wregex(LR"(<(.*?),(.*?)>)"), L"$1");
		strReplace(ws, L"^", L"");
		static std::wstring last;
		if (startWith(ws, last))
		{
			auto _ = ws.substr(last.size(), ws.size() - last.size());
			last = ws;
			ws = _;
		}
		else
			last = ws;
		buffer->from(WideStringToString(ws, 932));
	}
	void ULJM06397(TextBuffer *buffer, HookParam *hp)
	{
		auto ws = StringToWideString(buffer->viewA(), 932).value();
		ws = std::regex_replace(ws, std::wregex(LR"(<(.*?),(.*?)>)"), L"$1");
		strReplace(ws, L"^", L"");
		buffer->from(WideStringToString(ws, 932));
	}
	void ULJM06129(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(#\w+?\[\d\])"), "");
		strReplace(s, "#n", "");
		buffer->from(s);
	}
	void FNPJH50247(TextBuffer *buffer, HookParam *hp)
	{
		static lru_cache<std::string> cache(3);
		auto s = buffer->strA();
		if (cache.touch(s))
			buffer->clear();
		else
		{
			s = std::regex_replace(s, std::regex("#C[0-9]{9}"), "");
			s = std::regex_replace(s, std::regex("#RUBS(.*?)#RUBE(.*?)#REND"), "$2");
			s = std::regex_replace(s, std::regex("#CDEF"), "");
			buffer->from(s);
		}
	}
	void ULJM06145(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(#Ruby\[(.*?),(.*?)\])"), "$1");
		s = std::regex_replace(s, std::regex("#[A-Za-z]+\\[(\\d*\\.)?\\d+\\]+"), "");
		strReplace(s, "#n", "");
		strReplace(s, "\x84\xbd", "!?");
		buffer->from(s);
	}
	void FULJM05690(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(#Kana\[(.*?),(.*?)\])"), "$1");
		strReplace(s, "#n", "");
		buffer->from(s);
	}
	void FULJM05889(TextBuffer *buffer, HookParam *)
	{
		auto text = reinterpret_cast<LPSTR>(buffer->buff);
		for (size_t i = 0; i < buffer->size;)
		{
			if (IsShiftjisLeadByte(text[i]))
			{
				i += 2;
				continue;
			}
			if (text[i] == '^')
				text[i] = '\n';

			i += 1;
		}
	}

	void NPJH50619F(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex("[\\r\\n]+"), "");
		s = std::regex_replace(s, std::regex("^(.*?)\\)+"), "");
		s = std::regex_replace(s, std::regex("#ECL+"), "");
		s = std::regex_replace(s, std::regex("(#.+?\\))+"), "");
		buffer->from(s);
	}

	void NPJH50505F(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex("#RUBS(#[A-Z0-9]+)*[^#]+"), "");
		s = std::regex_replace(s, std::regex("#FAMILY"), "$FAMILY");
		s = std::regex_replace(s, std::regex("#GIVE"), "$GIVE");
		s = std::regex_replace(s, std::regex("(#[A-Z0-9\\-]+)+"), "");
		s = std::regex_replace(s, std::regex("\\n+"), "");
		buffer->from(s);
	}

	void ULJM05441(hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
	{
		auto data = PPSSPP::emu_arg(context)[1];
		std::string s;
		while (*(DWORD *)data)
		{
			std::string_view s1 = (char *)data;
			s += s1;
			data += s1.size() + 1;
		}
		strReplace(s, "\n", "");
		buffer->from(s);
	}
	void QNPJH50909(hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
	{
		auto data = PPSSPP::emu_arg(context)[0];
		uintptr_t addr = PPSSPP::emu_addr(context, 0x08975110);
		if (0x6e87 == *(WORD *)data)
			return;
		if (0x000a == *(WORD *)data)
			return;
		buffer->from(addr + 0x20, *(DWORD *)(addr + 0x14) * 2);
	}
	void NPJH50908(TextBuffer *buffer, HookParam *hp)
	{
		CharFilter(buffer, L'\n');
		CharFilter(buffer, L'　');
	}
	void ULJM05433(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		static std::string last;
		if (startWith(s, last))
		{
			auto _ = s.substr(last.size(), s.size() - last.size());
			last = s;
			s = _;
		}
		else
			last = s;
		buffer->from(s);
	}
	void ULJM06040_2(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(\x81k(.*?)\x81l(.*))"), "$1");
		buffer->from(s);
	}
	void ULJM05874(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "%K", 2);
		StringFilter(buffer, "%P", 2);
	}
	void ULJM05954(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "%K", 2);
		StringFilter(buffer, "%N", 2);
		StringFilter(buffer, "%P", 2);
		StringFilter(buffer, "\x81\x40", 2);
	}
	void ULJM06070(TextBuffer *buffer, HookParam *hp)
	{
		StringFilterBetween(buffer, "[", 1, "]", 1);
		if (!startWith(buffer->viewA(), "%C"))
			CharFilter(buffer, '\n');
		else
			StringFilter(buffer, "%C", 2);
	}
	void ULJM06040_1(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "%K", 2);
		StringFilter(buffer, "%P", 2);
		// StringFilterBetween(buffer, "\x81k", 2, "\x81l", 2);//〔ちなつ？〕〔直樹☆〕，人名，但可能不全，甚至包含剧透。想了一下还是留下吧
		StringFilter(buffer, "\x81\x99", 2); // ☆

		StringReplacer(buffer, "\x84\xa5", 2, "\x81\x5b", 2);
		StringReplacer(buffer, "\x84\xa7", 2, "\x81\x5b", 2);
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(\{(.*?)\}\[(.*?)\])"), "$1");
		buffer->from(s);
	}
	void NPJH00122(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		buffer->from(s.substr(14, s.size() - 15));
	}
	void ULJM05249(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		buffer->from(s.substr(0, s.find("\x81\x75")));
	}
	void ULJM05203(TextBuffer *buffer, HookParam *hp)
	{
		CharFilter(buffer, L'\n');
		static std::wstring last;
		auto s = buffer->viewW();
		if (s == last)
			return buffer->clear();
		last = s;
	}
	void ULJM05282(TextBuffer *buffer, HookParam *hp)
	{
		static std::wstring last;
		auto s = buffer->viewW();
		if (s == last)
			return buffer->clear();
		last = s;
	}
	void ULJM06343(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		static auto katakanaMap = std::map<std::wstring, std::wstring>{
			{L"｢", L"「"}, {L"｣", L"」"}, {L"ｧ", L"ぁ"}, {L"ｨ", L"ぃ"}, {L"ｩ", L"ぅ"}, {L"ｪ", L"ぇ"}, {L"ｫ", L"ぉ"}, {L"ｬ", L"ゃ"}, {L"ｭ", L"ゅ"}, {L"ｮ", L"ょ"}, {L"ｱ", L"あ"}, {L"ｲ", L"い"}, {L"ｳ", L"う"}, {L"ｴ", L"え"}, {L"ｵ", L"お"}, {L"ｶ", L"か"}, {L"ｷ", L"き"}, {L"ｸ", L"く"}, {L"ｹ", L"け"}, {L"ｺ", L"こ"}, {L"ｻ", L"さ"}, {L"ｼ", L"し"}, {L"ｽ", L"す"}, {L"ｾ", L"せ"}, {L"ｿ", L"そ"}, {L"ﾀ", L"た"}, {L"ﾁ", L"ち"}, {L"ﾂ", L"つ"}, {L"ﾃ", L"て"}, {L"ﾄ", L"と"}, {L"ﾅ", L"な"}, {L"ﾆ", L"に"}, {L"ﾇ", L"ぬ"}, {L"ﾈ", L"ね"}, {L"ﾉ", L"の"}, {L"ﾊ", L"は"}, {L"ﾋ", L"ひ"}, {L"ﾌ", L"ふ"}, {L"ﾍ", L"へ"}, {L"ﾎ", L"ほ"}, {L"ﾏ", L"ま"}, {L"ﾐ", L"み"}, {L"ﾑ", L"む"}, {L"ﾒ", L"め"}, {L"ﾓ", L"も"}, {L"ﾔ", L"や"}, {L"ﾕ", L"ゆ"}, {L"ﾖ", L"よ"}, {L"ﾗ", L"ら"}, {L"ﾘ", L"り"}, {L"ﾙ", L"る"}, {L"ﾚ", L"れ"}, {L"ﾛ", L"ろ"}, {L"ﾜ", L"わ"}, {L"ｦ", L"を"}, {L"ﾝ", L"ん"}, {L"ｰ", L"ー"}, {L"ｯ", L"っ"}, {L"､", L"、"}, {L"ﾟ", L"？"}, {L"ﾞ", L"！"}, {L"･", L"…"}, {L"?", L"　"}, {L"｡", L"。"}, {L"\uF8F0", L""}, {L"\uFFFD", L""} // invalid (shift_jis A0 <=> EF A3 B0) | FF FD - F8 F0)
		};
		auto remap = [](std::string &s)
		{
			std::wstring result;
			auto ws = StringToWideString(s, 932).value();
			for (auto _c : ws)
			{
				std::wstring c;
				c.push_back(_c);
				if (katakanaMap.find(c) != katakanaMap.end())
				{
					result += katakanaMap[c];
				}
				else
					result += c;
			}
			result = std::regex_replace(result, std::wregex(LR"(\$\[(.*?)\$/(.*?)\$\])"), L"$1");
			result = std::regex_replace(result, std::wregex(LR"(\$C\[(.*?)\])"), L"");
			result = std::regex_replace(result, std::wregex(LR"(\$\w)"), L"");
			result = std::regex_replace(result, std::wregex(LR"(@)"), L"");
			return WideStringToString(result, 932);
		};
		buffer->from(remap(s));
	}
	void ULJM05758(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		static auto katakanaMap = std::map<std::wstring, std::wstring>{
			{L"｢", L"「"}, {L"｣", L"」"}, {L"ｧ", L"ぁ"}, {L"ｨ", L"ぃ"}, {L"ｩ", L"ぅ"}, {L"ｪ", L"ぇ"}, {L"ｫ", L"ぉ"}, {L"ｬ", L"ゃ"}, {L"ｭ", L"ゅ"}, {L"ｮ", L"ょ"}, {L"ｱ", L"あ"}, {L"ｲ", L"い"}, {L"ｳ", L"う"}, {L"ｴ", L"え"}, {L"ｵ", L"お"}, {L"ｶ", L"か"}, {L"ｷ", L"き"}, {L"ｸ", L"く"}, {L"ｹ", L"け"}, {L"ｺ", L"こ"}, {L"ｻ", L"さ"}, {L"ｼ", L"し"}, {L"ｽ", L"す"}, {L"ｾ", L"せ"}, {L"ｿ", L"そ"}, {L"ﾀ", L"た"}, {L"ﾁ", L"ち"}, {L"ﾂ", L"つ"}, {L"ﾃ", L"て"}, {L"ﾄ", L"と"}, {L"ﾅ", L"な"}, {L"ﾆ", L"に"}, {L"ﾇ", L"ぬ"}, {L"ﾈ", L"ね"}, {L"ﾉ", L"の"}, {L"ﾊ", L"は"}, {L"ﾋ", L"ひ"}, {L"ﾌ", L"ふ"}, {L"ﾍ", L"へ"}, {L"ﾎ", L"ほ"}, {L"ﾏ", L"ま"}, {L"ﾐ", L"み"}, {L"ﾑ", L"む"}, {L"ﾒ", L"め"}, {L"ﾓ", L"も"}, {L"ﾔ", L"や"}, {L"ﾕ", L"ゆ"}, {L"ﾖ", L"よ"}, {L"ﾗ", L"ら"}, {L"ﾘ", L"り"}, {L"ﾙ", L"る"}, {L"ﾚ", L"れ"}, {L"ﾛ", L"ろ"}, {L"ﾜ", L"わ"}, {L"ｦ", L"を"}, {L"ﾝ", L"ん"}, {L"ｰ", L"ー"}, {L"ｯ", L"っ"}, {L"､", L"、"}, {L"ﾟ", L"？"}, {L"ﾞ", L"！"}, {L"･", L"…"}, {L"?", L"？"}, {L"｡", L"。"}};
		auto remap = [](std::string &s)
		{
			std::wstring result;
			auto ws = StringToWideString(s, 932).value();
			for (auto _c : ws)
			{
				std::wstring c;
				c.push_back(_c);
				if (katakanaMap.find(c) != katakanaMap.end())
				{
					result += katakanaMap[c];
				}
				else
					result += c;
			}
			return WideStringToString(result, 932);
		};
		buffer->from(remap(s));
	}
	void ULJM05634(TextBuffer *buffer, HookParam *hp)
	{
		static std::string last;
		auto s = buffer->strA();
		if (s == last)
			return buffer->clear();
		last = s;
	}
	void ULJS00169(TextBuffer *buffer, HookParam *hp)
	{
		CharFilter(buffer, '\n');
		static std::string last;
		auto s = buffer->strA();
		if (s == last)
			return buffer->clear();
		last = s;
	}
	void ULJM05477(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "@c", 2);
		auto s = buffer->strA();
		buffer->from(s.substr(1, s.size() - 2));
	}
	void ULJM06032(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "@n", 2);
		StringFilter(buffer, "\x81\x90", 2); // ＄
	}
	void ULJM05891(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "@n", 2);
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(@\w\d{4})"), "");
		buffer->from(s);
	}
	void ULJM05456(TextBuffer *buffer, HookParam *hp)
	{
		static std::string last;
		auto s = buffer->strA();
		if (endWith(last, s))
		{
			buffer->clear();
			last = s;
		}
		else
		{
			last = s;
			s = s.substr(0, s.size() - 1);
			s = std::regex_replace(s, std::regex(R"(\$\w\d{5})"), "$1");
			buffer->from(s);
		}
	}
	void NPJH50711(TextBuffer *buffer, HookParam *hp)
	{
		//%(モーターパラグライダー%*モーターパラグライダー%)
		auto s = buffer->strA();
		s = std::regex_replace(s, std::regex(R"(%\((.*?)%\*(.*?)%\))"), "$1");
		strReplace(s, "%!", "");
		buffer->from(s);
	}
	void ULJS00019(TextBuffer *buffer, HookParam *hp)
	{
		static std::string last;
		auto s = buffer->strA();
		if (endWith(last, s))
		{
			buffer->clear();
			last = s;
		}
		else
		{
			last = s;
			strReplace(s, "/K/L", "");
			strReplace(s, "/t", "");
			strReplace(s, "\n", "");
			std::string _s;
			for (int i = 0; i < s.size() - 1;)
			{
				if (s[i + 1] == 'T' && s[i] == '/')
				{
					i += 4;
				}
				else
				{
					_s += s[i];
					i++;
				}
			}
			_s += s[s.size() - 1];
			buffer->from(_s);
		}
	}
	void NPJH50380(TextBuffer *buffer, HookParam *hp)
	{
		static std::wstring last;
		static int lastj = 0;
		auto s = buffer->strW();
		if (!last.size())
		{
			buffer->clear();
		}
		else
		{
			if (s[0] != last[0])
				lastj = 0;
			int j = s.size() - 1;
			for (; (j >= 0) && (last[j] == s[j]); j--)
				;
			j += 1;
			buffer->from(s.substr(lastj, j - lastj));
			lastj = j;
		}
		last = s;
	}
	void ULJM05741(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		s = s.substr(0, s.size() - 2);
		if (s.find('#') != s.npos)
			return buffer->clear();
		buffer->from(s);
	}
	void ULJM05109(TextBuffer *buffer, HookParam *hp)
	{
		StringFilter(buffer, "$$", 2);
	}
	void ULJM06258_2(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strA();
		auto _ = strSplit(s, "\n");
		if (_.size() < 2)
			return buffer->clear();
		buffer->from(_[1]);
	}
	void ULJM06258(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		if (s.find("#n") == s.npos)
			return buffer->clear();
		ULJM05943F(buffer, hp);
	}
	void ULJM05823_2(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		if (s.find("#n") != s.npos)
			return buffer->clear();
	}
	void ULJM05725(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->viewA();
		if (s[0] == '#')
			return buffer->clear();
	}
	void ULJM06346(TextBuffer *buffer, HookParam *hp)
	{
		auto s = buffer->strW();
		static bool x = false;
		if (s.find(L"$playerName;") != s.npos)
		{
			x = true;
			strReplace(s, L"$playerName;", L"華");
		}
		else if (x && s == L"華")
		{
			x = false;
			return buffer->clear();
		}
		buffer->from(s);
	}
	void ULJM06200(TextBuffer *buffer, HookParam *hp)
	{
		NPJH50899(buffer, hp);
		auto s = buffer->strA();
		s = s.substr(0, 2) + std::regex_replace(s.substr(2, s.size() - 4), std::regex(R"(\x81\x77(.*?)\x81\x78)"), "$1") + s.substr(s.size() - 2);
		buffer->from(s);
	}
	std::unordered_map<uintptr_t, emfuncinfo> emfunctionhooks = {
		// Solomon's Ring ～地の章～  ULJM06204
		// Solomon's Ring ～水の章～  ULJM06203
		// Solomon's Ring ～風の章～  ULJM06202
		{0x88052C0, {0, 2, 0, 0, ULJM06200, "ULJM0620[234]"}},
		// Solomon's Ring ～火の章～
		{0x88061B0, {0, 2, 0, 0, ULJM06200, "ULJM06200"}},
		// 神なる君と
		{0x888F054, {0, 0, 0, 0, ULJM06289, "ULJM05975"}},
		// DEARDROPS DISTORTION
		{0x8814110, {USING_CHAR | DATA_INDIRECT, 4, 0, 0, 0, "ULJM05819"}},
		// 流行り神ＰＯＲＴＡＢＬＥ
		{0x88081cc, {0, 7, 0, 0, 0, "ULJS00035"}}, // 这三作都是单字符不断刷新，需要用比较复杂的处理
		// 流行り神２ＰＯＲＴＡＢＬＥ
		{0x883EAD0, {0, 0, 0, 0, 0, "ULJS00149"}},
		// 流行り神３
		{0x885CB50, {0, 3, 0, 0, 0, "ULJS00204"}},
		// 死神と少女
		{0x883bf34, {0, 1, 0, 0, ULJS00403_filter, "ULJS00403"}},
		// アマガミ
		{0x0886775c, {0, 0, 0, ULJS00339, 0, "ULJS00339"}}, // String.length()
		// 世界でいちばんNG（だめ）な恋
		{0x8814adc, {0, 0, 0, 0, NPJH50909_filter, "ULJM05878"}}, // name + dialouge
		{0x8850b2c, {0, 0, 0, 0, NPJH50909_filter, "ULJM05878"}}, // onscreen toast
		// Dunamis15
		{0x0891D72C, {CODEC_UTF8, 0, 0, 0, ULJM06119_filter, "ULJM06119"}},
		// 金色のコルダ３ AnotherSky feat.神南
		{0x883C940, {0, 0, 0, ULJM05428, 0, "NPJH50845"}},
		// 金色のコルダ３ フルボイス Special
		{0x8A731B0, {0, 1, 0, ULJM05428, 0, "NPJH50821"}},
		// 金色のコルダ３ AnotherSky feat.至誠館
		{0x8A99BD8, {0, 1, 0, ULJM05428, 0, "NPJH50846"}},
		// 金色のコルダ３ AnotherSky feat.天音学園
		{0x8AAB770, {0, 1, 0, ULJM05428, 0, "NPJH50847"}},
		// 金色のコルダ３
		{0x896C3B8, {0, 1, 0, ULJM05428, 0, "ULJM05624"}},
		// 金色のコルダ2 f
		{0x89b59dc, {0, 1, 0, ULJM05428, 0, "ULJM05428"}},
		// 金色のコルダ２ f アンコール
		{0x89D9FB0, {0, 1, 0, ULJM05428, 0, "ULJM05508"}},
		// 金色のコルダ
		{0x886162c, {0, 1, 0, ULJM05428, 0, "ULJM05054"}}, // dialogue: 0x886162c (x1), 0x889d5fc-0x889d520(a2) fullLine
		{0x8899e90, {0, 0, 0x3c, 0, 0, "ULJM05054"}},	   // name 0x88da57c, 0x8899ca4 (x0, oneTime), 0x8899e90
		// Sol Trigger
		{0x8952cfc, {CODEC_UTF8, 0, 0, 0, NPJH50619F, "NPJH50619"}}, // dialog
		{0x884aad4, {CODEC_UTF8, 0, 0, 0, NPJH50619F, "NPJH50619"}}, // description
		{0x882e1b0, {CODEC_UTF8, 0, 0, 0, NPJH50619F, "NPJH50619"}}, // system
		{0x88bb108, {CODEC_UTF8, 2, 0, 0, NPJH50619F, "NPJH50619"}}, // battle tutorial
		{0x89526a0, {CODEC_UTF8, 0, 0, 0, NPJH50619F, "NPJH50619"}}, // battle info
		{0x88bcef8, {CODEC_UTF8, 1, 0, 0, NPJH50619F, "NPJH50619"}}, // battle talk
		// Fate/EXTRA CCC
		{0x8958490, {0, 0, 0, 0, NPJH50505F, "NPJH50505"}},
		// Fate/EXTRA
		{0x88B87F0, {0, 6, 0, 0, FNPJH50247, "NPJH50247"}},
		// 神々の悪戯 InFinite
		{0x088630f8, {0, 0, 0, QNPJH50909, 0, "NPJH50909"}}, // text, choice (debounce trailing 400ms), TODO: better hook
		{0x0887813c, {0, 3, 4, 0, 0, "NPJH50909"}},			 // Question YN
		// アンジェリーク ルトゥール
		{0x88A4970, {CODEC_UTF16, 1, 0, 0, NPJH50908, "NPJH50908"}},
		// 月華繚乱ROMANCE
		{0x88eeba4, {0, 0, 0, 0, ULJM05943F, "ULJM05943"}}, // a0 - monologue text
		{0x8875e0c, {0, 1, 6, 0, ULJM05943F, "ULJM05943"}}, // a1 - dialogue text
		// My Merry May with be
		{0x886F014, {0, 3, 0, 0, FULJM05603, "ULJM05603"}},
		// コープスパーティー -THE ANTHOLOGY- サチコの恋愛遊戯♥Hysteric Birthday 2U
		{0x88517C8, {0, 1, 0, 0, FULJM05603, "ULJM06114"}},
		// 向日葵の教会と長い夏休み
		{0x881c444, {FULL_STRING, 0, 0, 0, 0, "ULJM06321"}}, // name+text,sjit,FULL_STRING to split name and text
		// ましろ色シンフォニー *mutsu-no-hana
		{0x8868AB8, {0, 0, 0, 0, FULJM05889, "ULJM05889"}},
		// シャイニング・ブレイド
		{0x8AA3B70, {0, 0xC, 0, 0, NPJH50530::T, "NPJH50530"}}, // text only
		{0x884DB44, {0, 1, 0, 0, NPJH50530::N, "NPJH50530"}},	// text+name
		// ティアーズ・トゥ・ティアラ 外伝 アヴァロンの謎 ＰＯＲＴＡＢＬＥ
		{0x890A4BC, {CODEC_UTF16, 1, 0, 0, FNPJH50243, "NPJH50243"}},
		// 薔薇ノ木ニ薔薇ノ花咲ク
		{0x881E560, {0, 1, 0, 0, 0, "ULJM05802"}},
		// D.C. Girl's Symphony Pocket ～ダ・カーポ～ ガールズシンフォニーポケット
		{0x883C77C, {0, 0, 0, 0, FULJM05690, "ULJM05690"}},
		// Ever17 -the out of infinity- Premium Edition
		{0x881AD64, {0, 0xd, 0, 0, 0, "ULJM05437"}},
		// １２時の鐘とシンデレラ～Halloween Wedding～
		{0x882A650, {0, 1, 0, 0, 0, "ULJM06023"}},
		// 0時の鐘とシンデレラ～Halloween Wedding～ (instance: 2)
		{0x8855CA0, {0, 1, 0, 0, 0, "ULJM06272"}},
		// セブンスドラゴン２０２０
		{0x88847A0, {CODEC_UTF8, 1, 0, 0, FNPJH50459, "NPJH50459"}},
		// セブンスドラゴン２０２０-Ⅱ
		{0x8889CCC, {CODEC_UTF8, 1, 0, 0, FNPJH50459, "NPJH50716"}}, // 会有两三条后续文本都被一次性提取到。
		// マイナスエイト
		{0x88DC218, {0, 0, 0, 0, FULJM05690, "ULJM06341"}},
		// ときめきメモリアル4
		{0x899a510, {0, 2, 0, 0, FNPJH50127, "NPJH50127"}},
		{0x88719dc, {0, 1, 0, 0, FNPJH50127, "NPJH50127"}},
		// オメルタ～沈黙の掟～ THE LEGACY
		{0x88861C8, {0, 3, 0, 0, 0, "ULJM06393"}},
		// L.G.S～新説 封神演義～
		{0x888A358, {0, 0, 0, 0, ULJM05943F, "ULJM06131"}}, // NAME+TEXT
		{0x88DB214, {0, 0, 0, 0, ULJM05943F, "ULJM06131"}}, // TEXT
		{0x889E970, {0, 0, 0, 0, ULJM05943F, "ULJM06131"}}, // NAME
		// S.Y.K ～蓮咲伝～ Portable
		{0x88FB080, {0, 0, 0, 0, ULJM05867_1, "ULJM05867"}}, // TEXT
		{0x88FB0B8, {0, 0, 0, 0, ULJM05867_2, "ULJM05867"}}, // NAME
		// 源狼 GENROH
		{0x8940DA8, {0, 1, 0, 0, ULJM06145, "ULJM06145"}}, // TEXT
		// 十鬼の絆
		{0x891AAAC, {0, 0, 0, 0, ULJM06129, "ULJM06129"}}, // text
		{0x886E094, {0, 0, 0, 0, ULJM06129, "ULJM06129"}}, // name+text
		// 十鬼の絆 花結綴り
		{0x886E354, {0, 0, 0, 0, ULJM06289, "ULJM06301"}}, // name+text
		// ティンクル☆くるせいだーす STARLIT BRAVE!!
		{0x88A94BC, {0, 4, 0, 0, 0, "ULJS00315"}}, // text
		// ティンクル☆くるせいだーす GoGo!
		{0x8822F24, {0, 0xe, 0, 0, 0, "ULJS00316"}}, // text
		// 明治東亰恋伽
		{0x886CA94, {0, 0, 0, 0, NPJH50900, "NPJH50808"}}, // text
		// 明治東亰恋伽 トワヰライト・キス
		{0x884DE44, {0, 0, 0, 0, NPJH50900, "NPJH50900"}}, // text
		// Never7 -the end of infinity-
		{0x88196F0, {0, 0xe, 0, 0, ULJM05433, "ULJM05433"}},
		// 青春はじめました！
		{0x880a744, {0, 0, 0, 0, ULJM05943F, "ULJM0630[23]"}}, // ULJM06302 & ULJM06303
		// アーメン・ノワール ポータブル
		{0x883b6a8, {0, 0, 0, 0, ULJM05943F, "ULJM06064"}},
		// デス・コネクション　ポータブル
		{0x882AEF4, {0, 0, 0, 0, ULJM05943F, "ULJM05823"}},
		{0x88B2464, {0, 0, 0, 0, ULJM05823_2, "ULJM05823"}}, // text+name->name
		// しらつゆの怪
		{0x888A26C, {0, 0, 0, 0, ULJM06289, "ULJM06289"}},
		// 新装版クローバーの国のアリス～Wonderful Wonder World～
		{0x8875E50, {0, 1, 0, 0, 0, "NPJH50894"}},
		// ダイヤの国のアリス～Wonderful Wonder World～
		{0x8857E3C, {0, 0, 0, 0, 0, "ULJM06216"}},
		// ダイヤの国のアリス～ Wonderful Mirror World ～
		{0x8855AE4, {0, 1, 0, 0, 0, "ULJM06295"}},
		// ハートの国のアリス～Wonderful Twin World～
		{0x8881CAC, {0, 1, 0, 0, 0, "NPJH50872"}},
		// 新装版 ハートの国のアリス～Wonderful Wonder World～
		{0x886B610, {0, 1, 0, 0, 0, "ULJM06332"}},
		// S・Y・K ～新説西遊記～ ポータブル
		{0x88DD918, {0, 0, 0, 0, ULJM05823_2, "ULJM05697"}}, // text+name->name
		{0x88DA420, {0, 4, 0, 0, ULJM05943F, "ULJM05697"}},
		// Glass Heart Princess
		{0x885FA30, {0, 0, 0, 0, ULJM05943F, "ULJM06196"}},
		// Glass Heart Princess:PLATINUM
		{0x885D4F0, {0, 0, 0, 0, ULJM05943F, "ULJM06309"}},
		// ウィル・オ・ウィスプ ポータブル
		{0x885DD04, {0, 0, 0, 0, ULJM05943F, "ULJM05447"}},
		// 華鬼 ～恋い初める刻 永久の印～
		{0x8829F14, {0, 4, 0, 0, ULJM05943F, "ULJM05847"}},
		{0x886D270, {0, 0, 0, 0, ULJM05823_2, "ULJM05847"}},
		// 華鬼 ～夢のつづき～
		{0x88406CC, {0, 0, 0, 0, ULJM05943F, "ULJM06048"}}, // text
		{0x885B7BC, {0, 0, 0, 0, ULJM05943F, "ULJM06048"}}, // name+text
		// サモンナイト３
		{0x89DCF90, {0, 6, 0, 0, NPJH50380, "NPJH50380"}},
		// サモンナイト４
		{0x89E7760, {0, 6, 0, 0, NPJH50380, "NPJH50410"}},
		// サモンナイト５
		{0x88C44CC, {CODEC_UTF8, 1, 0, 0, 0, "NPJH50696"}},
		// STEINS;GATE
		{0x8870320, {0, 3, 0, 0, ULJM06040_1, "ULJM05887"}},
		// Steins;Gate 比翼恋理のだーりん
		{0x8856968, {0, 4, 0, 0, ULJM06040_1, "ULJM06040"}},
		{0x889AD70, {0, 1, 0, 0, ULJM06040_2, "ULJM06040"}},
		// 鋼鉄のガールフレンド特別編ポータブル
		{0x882AAA4, {0, 1, 0, 0, ULJM05456, "ULJM05456"}},
		// 鋼鉄のガールフレンド2ndポータブル
		{0x8807FAC, {0, 1, 0, 0, ULJM05477, "ULJM05477"}},
		// アイドルマスターSP パーフェクトサン
		{0x8951A7C, {0, 1, 0, 0, ULJS00169, "ULJS00167"}},
		// アイドルマスターSP ワンダリングスター
		{0x8955E54, {0, 0, 0, 0, ULJS00169, "ULJS00168"}},
		// アイドルマスターSP ミッシングムーン
		{0x8951AE0, {0, 1, 0, 0, ULJS00169, "ULJS00169"}},
		// カヌチ 二つの翼
		{0x88158A0, {0, 0, 0, 0, ULJM05796, "ULJM0579[67]"}}, // ULJM05796 & ULJM05797
		// うたわれるもの PORTABLE
		{0x881CC54, {0, 0, 0, 0, ULJM05458, "ULJM05458"}},
		// とある科学の超電磁砲
		{0x88363A8, {FULL_STRING, 1, 0, 0, ULJS00354, "ULJS00354"}},
		// 幻想水滸伝　紡がれし百年の時
		{0x893FF00, {0, 0, 0, 0, NPJH50535, "NPJH50535"}},
		// アンチェインブレイズ レクス
		{0x88FD624, {CODEC_UTF8, 4, 0, 0, ULJM05756, "ULJM05756"}},
		// 密室のサクリファイス
		{0x88057D4, {0, 0xd, 0, 0, NPJH50224, "NPJH50224"}},
		// 密室のサクリファイス　～イトカ：ある閉鎖施設からの脱出～
		{0x8861A08, {0, 1, 0, 0, 0, "NPJH00065"}},
		// ココロコネクト ヨチランダム
		{0x8837BB8, {0, 1, 0, 0, 0, "NPJH50682"}},
		// 俺の妹がこんなに可愛いわけがない ポータブル が続くわけがない Ｄｉｓｃ１
		{0x88608B8, {CODEC_UTF16, 2, 0, 0, 0, "NPJH50568"}},
		// 俺の妹がこんなに可愛いわけがない ポータブル が続くわけがない Ｄｉｓｃ２
		{0x8860724, {CODEC_UTF16, 2, 0, 0, 0, "NPJH50569"}},
		// AIR
		{0x880C774, {CODEC_UTF16, 0, 0, 0, ULJM05282, "ULJM05282"}},
		// 蝶の毒 華の鎖～大正艶恋異聞～
		{0x883451C, {0, 0, 0, 0, ULJM06343, "ULJM06343"}},
		// Black Robinia
		{0x8850800, {0, 0, 0, 0, 0, "NPJH50394"}},
		// ワンド　オブ　フォーチュン　ポータブル
		{0x88878A0, {0, 0, 0, 0, ULJM05943F, "ULJM05689"}},
		// Remember11 -the age of infinity-
		{0x881BECC, {0, 0, 0, 0, 0, "ULJM05444"}},
		// のーふぇいと！ ～only the power of will～
		{0x889A888, {0, 0, 0, 0, ULJM05610, "ULJM05610"}},
		// 薄桜鬼 黎明録 ポータブル
		{0x88AA0FC, {0, 0, 0, 0, ULJM05943F, "ULJM05917"}},
		{0x88ABC14, {0, 0, 0, 0, ULJM05823_2, "ULJM05917"}},
		// 薄桜鬼 遊戯録
		{0x8850720, {0, 0, 0, 0, ULJM05943F, "ULJM05663"}},
		{0x884AB78, {0, 0, 0, 0, ULJM05823_2, "ULJM05663"}},
		// 薄桜鬼 遊戯録弐　祭囃子と隊士達
		{0x883E84C, {0, 1, 0, 0, ULJM05943F, "ULJM06165"}},
		// 裏語 薄桜鬼
		{0x885DECC, {0, 0, 0, 0, ULJM06281, "ULJM06281"}},
		// 裏語 薄桜鬼～暁の調べ～
		{0x88BDFB0, {0, 1, 0, 0, ULJM06281, "ULJM06373"}},
		// メモリーズオフ ゆびきりの記憶 ふたりの風流庵
		{0x8863D5C, {0, 3, 0, 0, ULJM05874, "ULJM05874"}},
		// メモリーズオフ ゆびきりの記憶
		{0x88A50B0, {0, 1, 0, 0, ULJM06040_1, "ULJM05875"}},
		// ＣＬＡＮＮＡＤ　光見守る坂道で　上巻
		{0x8850950, {0, 0xC, 0, 0, 0, "NPJH50266"}},
		// ＣＬＡＮＮＡＤ　光見守る坂道で　下巻
		{0x8853844, {0, 0xC, 0, 0, 0, "NPJH50273"}},
		// CLANNAD
		{0x880F240, {CODEC_UTF16, 0, 0, 0, ULJM05282, "ULJM0533[89]"}}, // ULJM05338 & ULJM05339
		// 東京鬼祓師　鴉乃杜學園奇譚
		{0x89F25C8, {0, 1, 0, 0, NPJH50215, "NPJH50215"}},
		// 雅恋 ～MIYAKO～
		{0x8812514, {0, 1, 0, 0, ULJM05770, "ULJM05770"}},
		// 雅恋 ～MIYAKO～ あわゆきのうたげ
		{0x8811368, {CODEC_UTF8, 1, 0, 0, ULJM06070, "ULJM06070"}},
		// ナルキッソス～もしも明日があるなら～Portable
		{0x8857B28, {0, 1, 0, 0, FULJM05603, "ULJM05674"}},
		// 未来日記　－１３人目の日記所有者－
		{0x884C30C, {0, 0, 0, 0, ULJM05565, "ULJM05565"}}, // 切换场景时会有很多辣鸡文本
		// CHAOS;HEAD らぶChu☆Chu!
		{0x88B5AD8, {0, 0xe, 0, 0, ULJM05821, "ULJM05821"}},
		// 涼宮ハルヒの約束
		{0x882C6B4, {0, 6, 0, 0, ULJS00124, "ULJS00124"}},
		// code_18
		{0x884B8B8, {0, 0, 0, 0, ULJM05821, "ULJM05936"}},
		// ユア・メモリーズオフ
		{0x88EF260, {0, 1, 0, 0, FULJM05603, "ULJM05435"}},
		// 華ヤカ哉、我ガ一族
		{0x885138C, {FULL_STRING, 1, 0, 0, ULJM05691, "ULJM05691"}},
		// 華ヤカ哉、我ガ一族 キネマモザイク
		{0x8816CE0, {FULL_STRING, 1, 0, 0, ULJM05691, "ULJM05998"}},
		// 華ヤカ哉、我ガ一族 黄昏ポウラスタ
		{0x889DD34, {0, 3, 0, 0, ULJM05691, "ULJM06263"}},
		// マザーグースの秘密の館～Nursery Rhymes for you～
		{0x8815B34, {0, 1, 0, 0, ULJS00124, "ULJM05892"}},
		// マザーグースの秘密の館～BLUE LABEL～
		{0x8831BB4, {0, 1, 0, 0, 0, "ULJM05950"}},
		// 百鬼夜行～怪談ロマンス～
		{0x884A634, {0, 1, 0, 0, 0, "ULJM06184"}},
		// 逢魔時～怪談ロマンス～
		{0x8833C64, {0, 1, 0, 0, ULJS00124, "ULJM06039"}},
		// 百物語～怪談ロマンス～
		{0x8843458, {0, 1, 0, 0, ULJS00124, "ULJM06323"}},
		// 黄昏時～怪談ロマンス～
		{0x8841A98, {0, 8, 0, 0, ULJS00124, "ULJM06235"}},
		// アブナイ★恋の捜査室
		{0x8842F84, {0, 1, 0, 0, 0, "ULJM06050"}},
		// ネオ アンジェリークSpecial
		{0x8867018, {0, 1, 0, 0, ULJS00124, "ULJM05374"}},
		// 遙かなる時空の中で～八葉抄～
		{0x88C1290, {0, 2, 0, 0, ULJS00124, "ULJM06252"}}, // 必须按一下按钮，才能显示
		// 遙かなる時空の中で２
		{0x88C0410, {0, 2, 0, 0, ULJM05019, "ULJM05019"}},
		// 遙かなる時空の中で３ with 十六夜記 愛蔵版
		{0x89024C8, {0, 0, 0, ULJM05441, 0, "ULJM05441"}},
		// 遙かなる時空の中で４ 愛蔵版
		{0x8955CE0, {0, 0, 0, ULJM05810, 0, "ULJM05810"}},
		// 遙かなる時空の中で５ 風花記
		{0x8B0449C, {0, 1, 0, 0, ULJS00124, "ULJM06025"}},
		// 遙かなる時空の中で６
		{0x89FD41C, {0, 0xf, 0, 0, NPJH50901, "NPJH50901"}},
		// SNOW BOUND LAND
		{0x88D6180, {0, 0, 0, 0, ULJM05943F, "ULJM06328"}}, // t
		{0x888AD68, {0, 0, 0, 0, ULJM05943F, "ULJM06328"}}, // n+t
		// Confidential Money ～300日で3000万ドル稼ぐ方法～
		{0x881BD00, {CODEC_UTF16, 1, 0, ULJM06143, 0, "ULJM06143"}},
		{0x882555C, {CODEC_UTF16, 2, 0, ULJM06143_1, 0, "ULJM06143"}},
		// アルカナ・ファミリア 幽霊船の魔術師
		{0x881A214, {0, 0, 0, 0, ULJM06032, "ULJM06032"}},
		// アルカナ・ファミリア ２
		{0x887493C, {0, 0, 0, 0, ULJM06032, "ULJM06291"}},
		// 里見八犬伝　八珠之記
		{0x887FF84, {0, 1, 0, 0, 0, "NPJH50858"}},
		// 里見八犬伝～村雨丸之記～
		{0x88750C0, {0, 1, 0, 0, NPJH50899, "NPJH50899"}},
		// 里見八犬伝～浜路姫之記～
		{0x886C750, {0, 1, 0, 0, NPJH50899, "NPJH50885"}},
		// 大正鬼譚～言ノ葉櫻～
		{0x88851E8, {0, 1, 0, 0, 0, "NPJH50886"}},
		// 白華の檻～緋色の欠片４～
		{0x88FE8C0, {0, 0, 0, 0, ULJM05823_2, "ULJM06167"}},
		{0x894672C, {0, 4, 0, 0, ULJM06167, "ULJM06167"}},
		// 白華の檻 ～緋色の欠片４～ 四季の詩
		{0x8851EA0, {0, 0, 0, 0, ULJM06266, "ULJM06314"}},
		{0x88E33E0, {0, 0, 0, 0, ULJM05943F, "ULJM06314"}},
		// 蒼黒の楔 緋色の欠片3 明日への扉
		{0x894C93C, {0, 1, 0, 0, ULJM06289, "ULJM06072"}},
		// 真・翡翠の雫 緋色の欠片２ ポータブル
		{0x887CEAC, {0, 0, 0, 0, ULJM06289, "ULJM05725"}},
		{0x8876794, {0, 0, 0, 0, ULJM05725, "ULJM05725"}},
		// 緋色の欠片ポータブル
		{0x88665E4, {0, 0, 0, 0, ULJM05943F, "ULJM05399"}},
		{0x8858770, {0, 4, 0, 0, ULJM05943F, "ULJM05399"}},
		// アラビアンズ・ダウト
		{0x88406FC, {0, 0, 0, 0, 0, "NPJH50834"}},
		// いざ、出陣！恋戦 第二幕 ～甲斐編～
		{0x8945C20, {CODEC_UTF16, 1, 0, 0, ULJM06346, "ULJM06346"}},
		{0x8804950, {CODEC_UTF16, 1, 0, 0, ULJM06346, "ULJM06347"}},
		// AMNESIA
		{0x88DABC4, {0, 0, 0, 0, ULJM06266, "ULJM05931"}},
		{0x88C9D44, {0, 1, 0, 0, ULJM05943F, "ULJM05931"}},
		// AMNESIA LATER
		{0x8880448, {0, 1, 0, 0, ULJM05823_2, "ULJM06044"}},
		{0x88F44B4, {0, 1, 0, 0, ULJM05943F, "ULJM06044"}},
		// AMNESIA CROWD
		{0x8912D30, {0, 0, 0, 0, ULJM06266, "ULJM06266"}},
		{0x890088C, {0, 1, 0, 0, ULJM05943F, "ULJM06266"}},
		// 宵夜森ノ姫
		{0x884C7C8, {0, 1, 0, ULJS00124_1, 0, "ULJM06394"}},
		// CLOCK ZERO ～終焉の一秒～ Portable
		{0x886F114, {0, 3, 0, 0, ULJM06289, "ULJM05945"}},
		// BLACK WOLVES SAGA -Last Hope-
		{0x883131C, {0, 1, 0, 0, ULJM06220, "ULJM06220"}},
		{0x8831324, {0, 1, 0, 0, ULJM06266, "ULJM06220"}},
		// クロノスタシア
		{0x8812600, {0, 1, 0, 0, ULJS00124, "ULJM06359"}},
		// フォトカノ
		{0x88F2030, {0, 1, 0, 0, ULJS00124, "ULJS00378"}},
		// マーメイド・ゴシック
		{0x888557C, {0, 1, 0, 0, 0, "NPJH50892"}},
		// your diary+
		{0x884D740, {0, 1, 0, 0, NPJH50831, "NPJH50831"}},
		{0x8829364, {0, 1, 0, 0, NPJH50831_1, "NPJH50831"}},
		// シャイニング・ハーツ
		{0x895A470, {CODEC_UTF16, 1, 0, 0, ULJM05282, "NPJH50342"}},
		// スクールランブル ～姉さん事件です！～
		{0x882C1C0, {0, 1, 0, 0, ULJS00019, "ULJS00019"}},
		// 水平線まで何マイル？ - ORIGINAL FLIGHT -
		{0x8922D2C, {0, 3, 0, 0, NPJH50711, "NPJH50711"}},
		// ガーネット・クレイドル ポータブル ～鍵の姫巫女～
		{0x88970F4, {0, 0, 0, 0, ULJM05823_2, "ULJM05858"}},
		{0x8873EB4, {0, 3, 0, 0, ULJM05943F, "ULJM05858"}},
		// さかあがりハリケーン Portable
		{0x880AF50, {0, 0, 0, 0, ULJM05891, "ULJM05891"}},
		// 快盗天使ツインエンジェル～時とセカイの迷宮～
		{0x880838C, {0, 1, 0, 0, ULJS00124, "ULJM05908"}},
		// テガミバチ　こころ紡ぐ者へ
		{0x883172C, {CODEC_UTF16, 1, 0, 0, ULJM05587_1, "ULJM05587"}},
		{0x88316F8, {CODEC_UTF16, 1, 0, 0, ULJM05587_2, "ULJM05587"}},
		// 闇からのいざない TENEBRAE I
		{0x88143A0, {CODEC_UTF16, 2, 0, 0, ULJM06147, "ULJM06147"}},
		// ＧＡ 芸術科アートデザインクラス Slapstick WONDERLAND
		{0x8858E44, {0, 0, 0, 0, ULJS00124, "ULJM05672"}},
		// この部室は帰宅しない部が占拠しました。ぽーたぶる　学園ドッグ・イヤー編
		{0x88F91DC, {0, 2, 0, 0, FNPJH50127, "ULJM06110"}},
		// 夏空のモノローグ portable
		{0x88756D0, {0, 3, 0, 0, NPJH50831, "ULJM06261"}},
		// 月影の鎖　～狂爛モラトリアム～
		{0x882CAE8, {0, 4, 0, 0, ULJS00579, "ULJS00599"}},
		// 月影の鎖　～錯乱パラノイア～
		{0x882C010, {0, 4, 0, 0, ULJS00579, "ULJS00579"}},
		// リアルロデ PORTABLE
		{0x886A92C, {0, 0, 0, 0, ULJM05657, "ULJM05657"}},
		// Princess Evangile
		{0x88506d0, {CODEC_UTF16, 2, 0, 0, ULJM06036_filter, "ULJM06036"}},
		// Princess Arthur
		{0x8841D10, {0, 0xE, 0, 0, ULJM06258_2, "ULJM06258"}}, // name+text,显示完后
		{0x88A844C, {0, 1, 0, 0, ULJM06258, "ULJM06258"}},	   // text
		// アルコバレーノ！ポータブル
		{0x88AFECC, {0, 4, 0, 0, ULJM05943F, "ULJM05609"}},
		// ヒイロノカケラ 新玉依姫伝承 ポータブル
		{0x883FA7C, {0, 4, 0, 0, ULJM05943F, "ULJM05741"}},
		{0x8813130, {0, 0xc, 0, 0, ULJM05741, "ULJM05741"}},
		// 十三支演義 ～偃月三国伝～
		{0x891BC1C, {0, 0, 0, 0, ULJM06289, "ULJM06090"}},
		// さくらさくら-HARU URARA-
		{0x8817C98, {0, 1, 0, 0, ULJM05758, "ULJM05758"}},
		// どきどきすいこでん
		{0x88A8FC8, {0, 1, 0, 0, ULJS00124, "ULJS00380"}},
		// Jewelic Nightmare
		{0x888BF24, {0, 0, 0, 0, ULJM06289, "ULJM06326"}},
		// 怪盗アプリコット ポータブル
		{0x8823474, {0, 1, 0, 0, 0, "ULJM05276"}},
		{0x8823DF0, {0, 1, 0, 0, ULJM05276, "ULJM05276"}},
		// EtudePrologue ポータブル
		{0x881D160, {0, 0, 0, 0, ULJM05276, "ULJM05252"}},
		{0x8822894, {0, 0, 0, 0, ULJM05276, "ULJM05252"}},
		{0x881CA64, {0, 6, 0, 0, ULJM05276, "ULJM05252"}},
		// for Symphony ポータブル
		{0x8821B50, {0, 1, 0, 0, ULJM05276, "ULJM05258"}},
		{0x881C354, {0, 0, 0, 0, ULJM05276, "ULJM05258"}},
		{0x881BC74, {0, 1, 0, 0, ULJM05276, "ULJM05258"}},
		// Kanon
		{0x880B6E8, {CODEC_UTF16, 0, 0, 0, ULJM05203, "ULJM05203"}},
		// Ｌｉｔｔｌｅ　Ａｉｄ　ポータブル
		{0x8822B6C, {0, 1, 0, 0, 0, "ULJM05249"}},
		{0x8823314, {0, 1, 0, 0, ULJM05249, "ULJM05249"}},
		// とびたて！超時空トラぶる花札大作戦
		{0x8842E48, {0, 2, 0, 0, NPJH00122, "NPJH00122"}},
		// ふしぎ遊戯 玄武開伝 外伝 鏡の巫女
		{0x884FC48, {0, 0, 0, 0, ULJM05823_2, "ULJM05175"}},
		{0x889B8D4, {0, 0, 0, 0, ULJM06258, "ULJM05175"}},
		// サクラ大戦１＆２
		{0x8A7F814, {0, 6, 0, 0, ULJM05109, "ULJM05109"}}, // 其一。不支持其二
		// 白銀のカルと蒼空の女王
		{0x8859358, {0, 3, 0, 0, ULJM05954, "ULJM05954"}},
		{0x884F338, {0, 0, 0, 0, ULJM05954, "ULJM05954"}},
		// カレイドイヴ
		{0x8837E98, {0, 3, 0, 0, ULJM06397, "ULJM06397"}},
		// 天神乱漫 Happy Go Lucky !!
		{0x885C730, {0, 0xe, 0, 0, ULJM05634, "ULJM05634"}},
		// 学園K -Wonderful School Days-
		{0x887F838, {0, 1, 0, 0, ULJM06378, "ULJM06378"}},
		// 学☆王 -THE ROYAL SEVEN STARS- +METEOR
		{0x880E458, {0, 0, 0, 0, NPJH50754, "NPJH50754"}},
		// 黒雪姫～スノウ・マジック～
		{0x8886480, {0, 1, 0, 0, 0, "NPJH50888"}},
		// ロミオVSジュリエット
		{0x887B4A4, {0, 1, 0, 0, 0, "ULJM06318"}},
		// うたの☆プリンスさまっ♪All Star After Secret
		{0x885F3C0, {0, 3, 0, NPJH50902, 0, "NPJH50902"}},
		// スカーレッドライダーゼクス
		{0x8863104, {0, 1, 0, 0, ULJM06006, "ULJM06006"}},
		// スカーレッドライダーゼクス　スターダストラバーズ
		{0x8862D80, {0, 1, 0, 0, ULJM06006, "ULJM06007"}},
	};

}