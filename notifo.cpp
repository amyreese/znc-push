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

		// BASIC auth string, needs to be encoded each time username/secret is changed
		CString notifo_auth;

		// Host and URL to send messages to
		CString notifo_host;
		CString notifo_url;

		// User agent to use
		CString user_agent;

		// User object
		CUser *user;

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

			// Current user
			user = GetUser();

			// Notifo user account and secret
			options["username"] = "";
			options["secret"] = "";

			// Notification conditions
			options["client_count_less_than"] = "0";
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
		 * Check how many clients are connected to ZNC.
		 *
		 * @return Number of connected clients
		 */
		unsigned int client_count()
		{
			return user->GetClients().size();
		}

		/**
		 * Check if the client_count condition is met.
		 *
		 * @return True if client_count is less than client_count_less_than or if client_count_less_than is zero
		 */
		bool client_count_less_than()
		{
			unsigned int value = options["client_count_less_than"].ToUInt();
			return value == 0 || client_count() < value;
		}

		/**
		 * Determine if the given message matches any highlight rules.
		 *
		 * @param message Message contents
		 * @return True if message matches a highlight
		 */
		bool highlight(const CString& message)
		{
			CNick nick = user->GetIRCNick();

			if (message.find(nick.GetNick()) != string::npos)
			{
				return true;
			}

			return false;
		}

		/**
		 * Determine when to notify the user of a channel message.
		 *
		 * @param nick Nick that sent the message
		 * @param channel Channel the message was sent to
		 * @param message Message contents
		 * @return Notification should be sent
		 */
		bool notify_channel(const CNick& nick, const CChan& channel, const CString& message)
		{
			if (!highlight(message))
			{
				return false;
			}

			if (!client_count_less_than())
			{
				return false;
			}

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
				CString value = GetNV(i->first);
				if (value != "")
				{
					options[i->first] = value;
				}
			}

			authencode();

			return true;
		}

		/**
		 * Handle channel messages.
		 *
		 * @param nick Nick that sent the message
		 * @param channel Channel the message was sent to
		 * @param message Message contents
		 */
		EModRet OnChanMsg(CNick& nick, CChan& channel, CString& message)
		{
			if (notify_channel(nick, channel, message))
			{
				CString title = "Highlight";
				CString msg = channel.GetName();
				msg += ": <" + nick.GetNick();
				msg += "> " + message;

				send_message(msg, title);
			}

			return CONTINUE;
		}

		/**
		 * Handle channel actions.
		 *
		 * @param nick Nick that sent the action
		 * @param channel Channel the message was sent to
		 * @param message Message contents
		 */
		EModRet OnChanAction(CNick& nick, CChan& channel, CString& message)
		{
			if (notify_channel(nick, channel, message))
			{
				CString title = "Highlight";
				CString msg = channel.GetName();
				msg += ": " + nick.GetNick();
				msg += " " + message;

				send_message(msg, title);
			}

			return CONTINUE;
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

			if (token_count < 1)
			{
				return;
			}

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
				if (token_count > 2)
				{
					PutModule("Usage: get [<option>]");
					return;
				}

				if (token_count < 2)
				{
					CTable table;

					table.AddColumn("Option");
					table.AddColumn("Value");

					for (MCString::iterator i = options.begin(); i != options.end(); i++)
					{
						table.AddRow();
						table.SetCell("Option", i->first);
						table.SetCell("Value", i->second);
					}

					PutModule(table);
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
			// STATUS command
			else if (action == "status")
			{
				CTable table;

				table.AddColumn("Condition");
				table.AddColumn("Status");

				table.AddRow();
				table.SetCell("Condition", "client_count");
				table.SetCell("Status", CString(client_count()));

				PutModule(table);
			}
			else
			{
				PutModule("Error: invalid command");
			}
		}
};

MODULEDEFS(CNotifoMod, "Send highlights and personal messages to a Notifo account")
