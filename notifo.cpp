/**
 * ZNC Notifo Module
 *
 * Allows the user to enter a Notifo user and API token, and sends
 * channel highlights and personal messages to Notifo.
 *
 * Copyright (c) 2011 John Reese
 * Licensed under the MIT license
 */

#define REQUIRESSL

#include "znc.h"
#include "Chan.h"
#include "User.h"
#include "Modules.h"

#if (!defined(VERSION_MAJOR) || !defined(VERSION_MINOR) || (VERSION_MAJOR == 0 && VERSION_MINOR < 72))
#error This module needs ZNC 0.072 or newer.
#endif

class CNotifoMod : public CModule
{
	public:
		MODCONSTRUCTOR(CNotifoMod) {}
		virtual ~CNotifoMod() {}

		virtual void OnModCommand(const CString& sCommand)
		{
			PutModule("Sending message...");
			sendmessage("foo");
		}

	protected:
		CString urlencode(const CString& str)
		{
			return str.Escape_n(CString::EASCII, CString::EURL);
		}

		void sendmessage(const CString& message)
		{
			CString crlf = "\r\n";

			CString auth = "foo:bar";

			CString post = "to=";
			post += "&msg=" + urlencode(message);
			post += "&label=" + urlencode(CString("ZNC"));
			post += "&title=" + urlencode(CString("New Message"));
			post += "&uri=";

			CString headers = "POST /index.php HTTP/1.1" + crlf;
			headers += "Host: notifo.leetcode.net" + crlf;
			headers += "Content-Type: application/x-www-form-urlencoded" + crlf;
			headers += "Content-Length: " + CString(post.length()) + crlf;
			headers += "User-Agent: zncnotifo" + crlf;
			headers += "Authorization: Basic " + auth.Base64Encode() + crlf;
			headers += crlf;
			headers += post + crlf;

			CSocket *sock = new CSocket(this);
			sock->Connect("notifo.leetcode.net", 443, true);
			sock->Write(headers);
			sock->Close(Csock::CLT_AFTERWRITE);
			AddSocket(sock);

			FILE *fh = fopen("/tmp/notifo.log", "a");
			fputs(headers.c_str(), fh);
			fclose(fh);
		}
};

MODULEDEFS(CNotifoMod, "Send highlights and personal messages to a Notifo account")
