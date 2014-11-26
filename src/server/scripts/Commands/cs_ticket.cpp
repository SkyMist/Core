/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
Name: ticket_commandscript
%Complete: 100
Comment: All ticket related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "AccountMgr.h"
#include "ObjectMgr.h"
#include "TicketMgr.h"

class ticket_commandscript : public CommandScript
{
public:
    ticket_commandscript() : CommandScript("ticket_commandscript") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand ticketResponseCommandTable[] =
        {
            { "append",         SEC_MODERATOR,      true,  &HandleGMTicketResponseAppendCommand,    "", NULL },
            { "appendln",       SEC_MODERATOR,      true,  &HandleGMTicketResponseAppendLnCommand,  "", NULL },
            { NULL,             0,                  false, NULL,                                    "", NULL }
        };
        static ChatCommand ticketCommandTable[] =
        {
            { "assign",         SEC_GAMEMASTER,     true,  &HandleGMTicketAssignToCommand,          "", NULL },
            { "close",          SEC_MODERATOR,      true,  &HandleGMTicketCloseByIdCommand,         "", NULL },
            { "closedlist",     SEC_MODERATOR,      true,  &HandleGMTicketListClosedCommand,        "", NULL },
            { "comment",        SEC_MODERATOR,      true,  &HandleGMTicketCommentCommand,           "", NULL },
            { "complete",       SEC_MODERATOR,      true,  &HandleGMTicketCompleteCommand,          "", NULL },
            { "delete",         SEC_ADMINISTRATOR,  true,  &HandleGMTicketDeleteByIdCommand,        "", NULL },
            { "escalate",       SEC_MODERATOR,      true,  &HandleGMTicketEscalateCommand,          "", NULL },
            { "escalatedlist",  SEC_GAMEMASTER,     true,  &HandleGMTicketListEscalatedCommand,     "", NULL },
            { "list",           SEC_MODERATOR,      true,  &HandleGMTicketListCommand,              "", NULL },
            { "onlinelist",     SEC_MODERATOR,      true,  &HandleGMTicketListOnlineCommand,        "", NULL },
            { "reset",          SEC_ADMINISTRATOR,  true,  &HandleGMTicketResetCommand,             "", NULL },
            { "response",       SEC_MODERATOR,      true,  NULL,                                    "", ticketResponseCommandTable },
            { "togglesystem",   SEC_ADMINISTRATOR,  true,  &HandleToggleGMTicketSystem,             "", NULL },
            { "unassign",       SEC_GAMEMASTER,     true,  &HandleGMTicketUnAssignCommand,          "", NULL },
            { "viewid",         SEC_MODERATOR,      true,  &HandleGMTicketGetByIdCommand,           "", NULL },
            { "viewname",       SEC_MODERATOR,      true,  &HandleGMTicketGetByNameCommand,         "", NULL },
            { NULL,             0,                  false, NULL,                                    "", NULL }
        };
        static ChatCommand commandTable[] =
        {
            { "ticket",         SEC_MODERATOR,      false, NULL,                                    "", ticketCommandTable },
            { NULL,             0,                  false, NULL,                                    "", NULL }
        };
        return commandTable;
    }

    static bool HandleGMTicketAssignToCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* ticketIdStr = strtok((char*)args, " ");
        uint32 ticketId = atoi(ticketIdStr);

        char* targetStr = strtok(NULL, " ");
        if (!targetStr)
            return false;

        std::string target(targetStr);
        if (!normalizePlayerName(target))
            return false;

        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Get target information
        uint64 targetGuid = sObjectMgr->GetPlayerGUIDByName(target.c_str());
        uint64 targetAccountId = sObjectMgr->GetPlayerAccountIdByGUID(targetGuid);
        uint32 targetGmLevel = AccountMgr::GetSecurity(targetAccountId, realmID);

        // Target must exist and have administrative rights
        if (!targetGuid || AccountMgr::IsPlayerAccount(targetGmLevel))
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETASSIGNERROR_A);
            return true;
        }

        // Ticket must not already be assigned to the same player.
        if (ticket->IsAssignedTo(targetGuid))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETASSIGNERROR_B, ticket->GetId());
            return true;
        }

        // Ticket must not already be assigned to the another player. Console can override though.
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL;
        if (player && ticket->IsAssignedNotTo(player->GetGUID()))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId(), target.c_str());
            return true;
        }

        // Assign ticket and save it.
        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetAssignedTo(targetGuid, AccountMgr::IsAdminAccount(targetGmLevel));
        ticket->SetResponse("Your ticket has been assigned to a GM and is being serviced.");
        ticket->SaveToDB(trans);

        // Update the last change time.
        sTicketMgr->UpdateLastChange();

        std::string msg = ticket->FormatMessageString(*handler, NULL, target.c_str(), NULL, NULL);
        handler->SendGlobalGMSysMessage(msg.c_str());

        return true;
    }

    static bool HandleGMTicketCloseByIdCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);

        // Ticket must exist and not be closed.
        if (!ticket || ticket->IsClosed())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Ticket should be assigned to the player who tries to close it. Console can override though.
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL;
        if (player && ticket->IsAssignedNotTo(player->GetGUID()))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETCANNOTCLOSE, ticket->GetId());
            return true;
        }

        sTicketMgr->CloseTicket(ticket->GetId(), player ? player->GetGUID() : -1);
        sTicketMgr->UpdateLastChange();

        std::string msg = ticket->FormatMessageString(*handler, player ? player->GetName() : "Console", NULL, NULL, NULL);
        handler->SendGlobalGMSysMessage(msg.c_str());

        // Inform the player that the ticket is closed (remove it).
        if (Player* submitter = ticket->GetPlayer())
        {
            if (submitter->IsInWorld())
            {
                sTicketMgr->SendTicketStatusUpdate(submitter->GetSession(), GMTICKET_RESPONSE_TICKET_DELETED);
                sTicketMgr->SendTicket(submitter->GetSession(), NULL);
            }
        }

        return true;
    }

    static bool HandleGMTicketCommentCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* ticketIdStr = strtok((char*)args, " ");
        uint32 ticketId = atoi(ticketIdStr);

        char* comment = strtok(NULL, "\n");
        if (!comment)
            return false;

        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);

        // Ticket must exist and not be closed / completed.
        if (!ticket || ticket->IsClosed() || ticket->IsCompleted())
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Cannot comment ticket assigned to someone else. Console excluded.
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL;
        if (player && ticket->IsAssignedNotTo(player->GetGUID()))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());
            return true;
        }

        // Set and save the ticket comment.
        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetComment(comment);
        ticket->SaveToDB(trans);

        // Update the last change time.
        sTicketMgr->UpdateLastChange();

        std::string msg = ticket->FormatMessageString(*handler, NULL, ticket->GetAssignedToName().c_str(), NULL, NULL);
        msg += handler->PGetParseString(LANG_COMMAND_TICKETLISTADDCOMMENT, player ? player->GetName() : "Console", comment);
        handler->SendGlobalGMSysMessage(msg.c_str());

        return true;
    }

    static bool HandleGMTicketListClosedCommand(ChatHandler* handler, char const* /*args*/)
    {
        sTicketMgr->ShowClosedList(*handler);
        return true;
    }

    static bool HandleGMTicketCompleteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);

        // Ticket must exist and not be closed / completed.
        if (!ticket || ticket->IsClosed() || ticket->IsCompleted())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Set the ticket as completed and the text to reflect the change, and save it.
        ticket->SetCompleted(true);
        ticket->SetResponse("Your ticket has been serviced and resolved.");

        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        ticket->SaveToDB(trans);
        CharacterDatabase.CommitTransaction(trans);

        // Update the last change time.
        sTicketMgr->UpdateLastChange();

        return true;
    }

    static bool HandleGMTicketDeleteByIdCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);

        // Ticket must exist.
        if (!ticket)
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Ticket must be closed.
        if (!ticket->IsClosed())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETCLOSEFIRST);
            return true;
        }

        std::string msg = ticket->FormatMessageString(*handler, NULL, NULL, NULL, handler->GetSession() ? handler->GetSession()->GetPlayer()->GetName() : "Console");
        handler->SendGlobalGMSysMessage(msg.c_str());

        // Force the player to abandon the ticket.
        if (Player* player = ticket->GetPlayer())
        {
            if (player->IsInWorld())
            {
                sTicketMgr->SendTicketStatusUpdate(player->GetSession(), GMTICKET_RESPONSE_TICKET_DELETED);
                sTicketMgr->SendTicket(player->GetSession(), NULL);
            }
        }

        // Remove the ticket.
        sTicketMgr->RemoveTicket(ticket->GetId());
        sTicketMgr->UpdateLastChange();

        return true;
    }

    static bool HandleGMTicketEscalateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);

        // Ticket must exists and not be closed / completed / assigned.
        if (!ticket || ticket->IsClosed() || ticket->IsCompleted() || ticket->GetEscalatedStatus() != TICKET_UNASSIGNED)
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Set the ticket in the priority queue and the text and save it.
        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetEscalatedStatus(TICKET_IN_ESCALATION_QUEUE);
        ticket->SetResponse("Your ticket is in the priority queue and will be serviced shortly.");
        ticket->SaveToDB(trans);

        // Update the last change time.
        sTicketMgr->UpdateLastChange();

        return true;
    }

    static bool HandleGMTicketListEscalatedCommand(ChatHandler* handler, char const* /*args*/)
    {
        sTicketMgr->ShowEscalatedList(*handler);
        return true;
    }

    static bool HandleGMTicketListCommand(ChatHandler* handler, char const* /*args*/)
    {
        sTicketMgr->ShowList(*handler, false);
        return true;
    }

    static bool HandleGMTicketListOnlineCommand(ChatHandler* handler, char const* /*args*/)
    {
        sTicketMgr->ShowList(*handler, true);
        return true;
    }

    static bool HandleGMTicketResetCommand(ChatHandler* handler, char const* /*args*/)
    {
        if (sTicketMgr->GetOpenTicketCount())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETPENDING);
            return true;
        }
        else
        {
            sTicketMgr->ResetTickets();
            handler->SendSysMessage(LANG_COMMAND_TICKETRESET);
        }

        return true;
    }

    static bool HandleToggleGMTicketSystem(ChatHandler* handler, char const* /*args*/)
    {
        bool status = !sTicketMgr->GetStatus();
        sTicketMgr->SetStatus(status);

        handler->PSendSysMessage(status ? LANG_ALLOW_TICKETS : LANG_DISALLOW_TICKETS);
        return true;
    }

    static bool HandleGMTicketUnAssignCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);

        // Ticket must exist and not be closed.
        if (!ticket || ticket->IsClosed())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Ticket must be assigned.
        if (!ticket->IsAssigned())
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETNOTASSIGNED, ticket->GetId());
            return true;
        }

        // Get security level of player, whom this ticket is assigned to
        uint32 security = SEC_PLAYER;
        Player* assignedPlayer = ticket->GetAssignedPlayer();

        if (assignedPlayer && assignedPlayer->IsInWorld())
            security = assignedPlayer->GetSession()->GetSecurity();
        else
        {
            uint64 guid = ticket->GetAssignedToGUID();
            uint32 accountId = sObjectMgr->GetPlayerAccountIdByGUID(guid);
            security = AccountMgr::GetSecurity(accountId, realmID);
        }

        // Check security. If no m_session present it means we're issuing this command from the console.
        uint32 mySecurity = handler->GetSession() ? handler->GetSession()->GetSecurity() : SEC_CONSOLE;
        if (security > mySecurity)
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETUNASSIGNSECURITY);
            return true;
        }

        // Set the ticket as unassigned and the text and save it.
        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetUnassigned();
        ticket->SetResponse("Your ticket will be serviced shortly.");
        ticket->SaveToDB(trans);

        // Update the last change time.
        sTicketMgr->UpdateLastChange();

        std::string msg = ticket->FormatMessageString(*handler, NULL, ticket->GetAssignedToName().c_str(),
            handler->GetSession() ? handler->GetSession()->GetPlayer()->GetName() : "Console", NULL);
        handler->SendGlobalGMSysMessage(msg.c_str());

        return true;
    }

    static bool HandleGMTicketGetByIdCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed() || ticket->IsCompleted())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Set the ticket as viewed and save it.
        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetViewed();
        ticket->SetResponse("Your ticket was viewed and is being serviced.");
        ticket->SaveToDB(trans);

        // Update the last change time.
        sTicketMgr->UpdateLastChange();

        handler->SendSysMessage(ticket->FormatMessageString(*handler, true).c_str());

        return true;
    }

    static bool HandleGMTicketGetByNameCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string name(args);
        if (!normalizePlayerName(name))
            return false;

        // Detect target's GUID
        uint64 guid = 0;
        if (Player* player = sObjectAccessor->FindPlayerByName(name.c_str()))
            guid = player->GetGUID();
        else
            guid = sObjectMgr->GetPlayerGUIDByName(name);

        // Target must exist.
        if (!guid)
        {
            handler->SendSysMessage(LANG_NO_PLAYERS_FOUND);
            return true;
        }

        // Ticket must exist.
        GmTicket* ticket = sTicketMgr->GetTicketByPlayer(guid);
        if (!ticket)
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Set the ticket as viewed and save it.
        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetViewed();
        ticket->SetResponse("Your ticket was viewed and is being serviced.");
        ticket->SaveToDB(trans);

        // Update the last change time.
        sTicketMgr->UpdateLastChange();

        handler->SendSysMessage(ticket->FormatMessageString(*handler, true).c_str());

        return true;
    }

    static bool _HandleGMTicketResponseAppendCommand(char const* args, bool newLine, ChatHandler* handler)
    {
        if (!*args)
            return false;

        char* ticketIdStr = strtok((char*)args, " ");
        uint32 ticketId = atoi(ticketIdStr);

        char* response = strtok(NULL, "\n");
        if (!response)
            return false;

        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed())
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Cannot add response to ticket, assigned to someone else. Console excluded.
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL;
        if (player && ticket->IsAssignedNotTo(player->GetGUID()))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());
            return true;
        }

        // Set the ticket text and save it.
        SQLTransaction trans = SQLTransaction(NULL);

        if (newLine)
            ticket->SetResponse(response);    // Makes a new response (form: new).
        else
            ticket->AppendResponse(response); // Puts the response over the old one (form: old + new).

        ticket->SaveToDB(trans);

        // Update the last change time.
        sTicketMgr->UpdateLastChange();

        return true;
    }

    static bool HandleGMTicketResponseAppendCommand(ChatHandler* handler, char const* args)
    {
        return _HandleGMTicketResponseAppendCommand(args, false, handler);
    }

    static bool HandleGMTicketResponseAppendLnCommand(ChatHandler* handler, char const* args)
    {
        return _HandleGMTicketResponseAppendCommand(args, true, handler);
    }
};

void AddSC_ticket_commandscript()
{
    new ticket_commandscript();
}
