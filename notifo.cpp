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

#define DEBUG_HOST 0
#define DEBUG_LOGGING 0

class CNotifoMod : public CModule
{
	protected:

		// Application name
		CString app;

		// Too lazy to add CString("\r\n\") everywhere
		CString crlf;

		// Host and URL to send messages to
		CString notifo_host;
		CString notifo_url;

		// User agent to use
		CString user_agent;

		// BASIC auth string, needs to be encoded each time username/secret is changed
		CString notifo_auth;

		// Configuration options
		MCString options;

	public:

		MODCONSTRUCTOR(CNotifoMod) {
			app = "ZNC";
			crlf = "\r\n";

#if DEBUG_HOST
			notifo_host = "notifo.leetcode.net";
			notifo_url = "/index.php";
#else
			notifo_host = "api.notifo.com";
			notifo_url = "/v1/send_notification";
#endif

			notifo_auth = "";
			user_agent = "ZNC To Notifo";

			// Notifo user account and secret
			options["username"] = "";
			options["secret"] = "";
		}
		virtual ~CNotifoMod() {}

	protected:

		/**
		 * Shorthand for encoding a string for a URL.
		 *
		 * @param str String to be encoded
		 * @return Encoded string
		 */
		CString urlencode(const CString& str)
		{
			return str.Escape_n(CString::EASCII, CString::EURL);
		}

		/**
		 * Re-encode the authentication credentials.
		 */
		void authencode()
		{
			// BASIC auth, base64-encoded username:password
			CString auth = options["username"] + CString(":") + options["secret"];
			notifo_auth = auth.Base64Encode_n();
		}

		/**
		 * Send a message to the currently-configured Notifo account.
		 * Requires (and assumes) that the user has already configured their
		 * username and API secret using the 'set' command.
		 *
		 * @param message Message to be sent to the user
		 * @param title Message title to use
		 */
		void send_message(const CString& message, const CString& title="New Message")
		{
			// POST body parameters for the request
			CString post = "to=" + urlencode(options["username"]);
			post += "&msg=" + urlencode(message);
			post += "&label=" + urlencode(app);
			post += "&title=" + urlencode(title);
			post += "&uri=" + urlencode(CString("http://notifo.leetcode.net/"));

			// Request headers and POST body
			CString request = "POST " + notifo_url + " HTTP/1.1" + crlf;
			request += "Host: " + notifo_host + crlf;
			request += "Content-Type: application/x-www-form-urlencoded" + crlf;
			request += "Content-Length: " + CString(post.length()) + crlf;
			request += "User-Agent: " + user_agent + crlf;
			request += "Authorization: Basic " + notifo_auth + crlf;
			request += crlf;
			request += post + crlf;

			// Create the socket connection, write to it, and add it to the queue
			CSocket *sock = new CSocket(this);
			sock->Connect(notifo_host, 443, true);
			sock->Write(request);
			sock->Close(Csock::CLT_AFTERWRITE);
			AddSocket(sock);

#if DEBUG_LOGGING
			// Log the HTTP request
			FILE *fh = fopen("/tmp/notifo.log", "a");
			fputs(request.c_str(), fh);
			fclose(fh);
#endif
		}

		/**
		 * Determine when to notify the user of a channel message.
		 *
		 * @param channel Channel the message was sent to
		 * @return Notification should be sent
		 */
		bool notify_channel(const CChan& channel)
		{
			return true;
		}

		/**
		 * Determine when to notify the user of a private message.
		 *
		 * @param nick Nick that sent the message
		 * @return Notification should be sent
		 */
		bool notify_pm(const CNick& nick)
		{
			return true;
		}

	protected:

		/**
		 * Handle the plugin being loaded.  Retrieve plugin config values.
		 *
		 * @param args Plugin arguments
		 * @param message Message to show the user after loading
		 */
		bool OnLoad(const CString& args, CString& message)
		{
			for (MCString::iterator i = options.begin(); i != options.end(); i++)
			{
				options[i->first] = GetNV(i->first);
			}

			authencode();

			return true;
		}

		/**
		 * Handle a private message.
		 *
		 * @param nick Nick that sent the message
		 * @param message Message contents
		 */
		EModRet OnPrivMsg(CNick& nick, CString& message)
		{
			if (notify_pm(nick))
			{
				CString title = "Private Message";
				CString msg = "From " + nick.GetNick();
				msg += ": " + message;

				send_message(msg, title);
			}

			return CONTINUE;
		}

		/**
		 * Handle a private action.
		 *
		 * @param nick Nick that sent the action
		 * @param message Message contents
		 */
		EModRet OnPrivAction(CNick& nick, CString& message)
		{
			if (notify_pm(nick))
			{
				CString title = "Private Message";
				CString msg = "* " + nick.GetNick();
				msg += " " + message;

				send_message(msg, title);
			}

			return CONTINUE;
		}

		/**
		 * Handle direct commands to the *notifo virtual user.
		 *
		 * @param command Command string
		 */
		void OnModCommand(const CString& command)
		{
			VCString tokens;
			int token_count = command.Split(" ", tokens, false);

			CString action = tokens[0].AsLower();

			// SET command
			if (action == "set")
			{
				if (token_count != 3)
				{
					PutModule("Usage: set <option> <value>");
					return;
				}

				CString option = tokens[1].AsLower();
				CString value = tokens[2];
				MCString::iterator pos = options.find(option);

				if (pos == options.end())
				{
					PutModule("Error: invalid option name");
				}
				else
				{
					options[option] = value;
					SetNV(option, value);

					authencode();
				}
			}
			// GET command
			else if (action == "get")
			{
				if (token_count != 2)
				{
					PutModule("Usage: get <option>");
					return;
				}

				CString option = tokens[1].AsLower();
				MCString::iterator pos = options.find(option);

				if (pos == options.end())
				{
					PutModule("Error: invalid option name");
				}
				else
				{
					PutModule(option + CString(": \"") + options[option] + CString("\""));
				}
			}
			// SEND command
			else if (action == "send")
			{
				CString message = command.Token(1, true, " ", true);
				send_message(message);
			}
			else
			{
				PutModule("Error: invalid command");
			}
		}
};

MODULEDEFS(CNotifoMod, "Send highlights and personal messages to a Notifo account")
