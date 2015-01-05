/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Language.h"
#include "WorldPacket.h"
#include "Common.h"
#include "ObjectMgr.h"
#include "TicketMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "Util.h"

void WorldSession::HandleGMTicketCreateOpcode(WorldPacket& recvData)
{
    // Don't accept tickets if the ticket queue is disabled. (Ticket UI is greyed out but not fully dependable)
    if (sTicketMgr->GetStatus() == GMTICKET_QUEUE_STATUS_DISABLED)
        return;

    if (GetPlayer()->getLevel() < sWorld->getIntConfig(CONFIG_TICKET_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_TICKET_REQ), sWorld->getIntConfig(CONFIG_TICKET_LEVEL_REQ));
        return;
    }

    GMTicketResponse response = GMTICKET_RESPONSE_CREATE_ERROR;

    // Player must not have an opened ticket.
    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
    {
        if (ticket->IsCompleted())
        {
            // Close and delete the ticket, creating a new one.
            sTicketMgr->CloseTicket(ticket->GetId(), GetPlayer()->GetGUID());
            sTicketMgr->SendTicketStatusUpdate(this, GMTICKET_RESPONSE_TICKET_DELETED);
            sTicketMgr->SendTicket(this, NULL);
            sTicketMgr->RemoveTicket(ticket->GetId());

            // Time to build the new ticket.
            GmTicket* newTicket = new GmTicket(GetPlayer(), recvData);
            sTicketMgr->AddTicket(newTicket);
            sTicketMgr->UpdateLastChange();

            sWorld->SendGMText(LANG_COMMAND_TICKETNEW, GetPlayer()->GetName(), newTicket->GetId());

            sTicketMgr->SendTicket(this, newTicket);

            response = GMTICKET_RESPONSE_CREATE_SUCCESS;
        }
        else
            response = GMTICKET_RESPONSE_ALREADY_EXIST;
    }
    else
    {
        GmTicket* newTicket = new GmTicket(GetPlayer(), recvData);
        sTicketMgr->AddTicket(newTicket);
        sTicketMgr->UpdateLastChange();

        sWorld->SendGMText(LANG_COMMAND_TICKETNEW, GetPlayer()->GetName(), newTicket->GetId());

        sTicketMgr->SendTicket(this, newTicket);

        response = GMTICKET_RESPONSE_CREATE_SUCCESS;
    }

    sTicketMgr->SendTicketStatusUpdate(this, response);
}

void WorldSession::HandleGMTicketUpdateOpcode(WorldPacket& recvData)
{
    size_t strLen = recvData.ReadBits(11);

    recvData.FlushBits();

    std::string message = recvData.ReadString(strLen);

    GMTicketResponse response = GMTICKET_RESPONSE_UPDATE_ERROR;

    // If the ticket is found, update it.
    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
    {
        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetMessage(message);
        ticket->SaveToDB(trans);

        sWorld->SendGMText(LANG_COMMAND_TICKETUPDATED, GetPlayer()->GetName(), ticket->GetId());

        sTicketMgr->SendTicket(this, ticket);
        sTicketMgr->UpdateLastChange();

        response = GMTICKET_RESPONSE_UPDATE_SUCCESS;
    }

    sTicketMgr->SendTicketStatusUpdate(this, response);
}

void WorldSession::HandleGMTicketDeleteOpcode(WorldPacket& /*recvData*/)
{
    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
    {
        sWorld->SendGMText(LANG_COMMAND_TICKETPLAYERABANDON, GetPlayer()->GetName(), ticket->GetId());

        sTicketMgr->CloseTicket(ticket->GetId(), GetPlayer()->GetGUID());
        sTicketMgr->SendTicketStatusUpdate(this, GMTICKET_RESPONSE_TICKET_DELETED);
        sTicketMgr->SendTicket(this, NULL);

        sTicketMgr->RemoveTicket(ticket->GetId());
        sTicketMgr->UpdateLastChange();
    }
}

void WorldSession::HandleGMTicketGetTicketOpcode(WorldPacket& /*recvData*/)
{
    SendQueryTimeResponse();

    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
        sTicketMgr->SendTicket(this, ticket);
    else
        sTicketMgr->SendTicket(this, NULL);
}

void WorldSession::HandleGMTicketGetWebTicketOpcode(WorldPacket& /*recvPacket*/)
{
}

void WorldSession::HandleGMTicketSystemStatusOpcode(WorldPacket& /*recvData*/)
{
    // Note: This only disables the ticket UI at client side and is not fully reliable.

    WorldPacket data(SMSG_GM_TICKET_SYSTEM_STATUS, 4);
    data << uint32(sTicketMgr->GetStatus() ? GMTICKET_QUEUE_STATUS_ENABLED : GMTICKET_QUEUE_STATUS_DISABLED);
    SendPacket(&data);
}

void WorldSession::HandleGMSurveySubmit(WorldPacket& recvData)
{
    // Just puts the survey in the database.

    uint32 nextSurveyID = sTicketMgr->GetNextSurveyID();

    uint32 mainSurvey;       // GMSurveyCurrentSurvey.dbc, column 1 (all 9) ref to GMSurveySurveys.dbc
    std::string mainComment; // Just a guess.

    recvData >> mainSurvey;

    // sub_survey1, r1, comment1, sub_survey2, r2, comment2, sub_survey3, r3, comment3, sub_survey4, r4, comment4, sub_survey5, r5, comment5, sub_survey6, r6, comment6, sub_survey7, r7, comment7, sub_survey8, r8, comment8, sub_survey9, r9, comment9, sub_survey10, r10, comment10,
    for (uint8 i = 0; i < 10; i++)
    {
        uint32 subSurveyId;      // Ref to i'th GMSurveySurveys.dbc field (all fields in that dbc point to fields in GMSurveyQuestions.dbc)
        uint8 rank;              // Probably some sort of ref to GMSurveyAnswers.dbc.
        std::string subComment;  // Comment ("Usage: GMSurveyAnswerSubmit(question, rank, comment)").

        recvData >> subSurveyId;
        if (!subSurveyId)
            break;

        recvData >> rank >> subComment;

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GM_SUBSURVEY);
        stmt->setUInt32(0, nextSurveyID);
        stmt->setUInt32(1, subSurveyId);
        stmt->setUInt32(2, rank);
        stmt->setString(3, subComment);
        CharacterDatabase.Execute(stmt);
    }

    recvData >> mainComment;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GM_SURVEY);
    stmt->setUInt32(0, GUID_LOPART(GetPlayer()->GetGUID()));
    stmt->setUInt32(1, nextSurveyID);
    stmt->setUInt32(2, mainSurvey);
    stmt->setString(3, mainComment);

    CharacterDatabase.Execute(stmt);
}

void WorldSession::HandleReportLag(WorldPacket& recvData)
{
    // Just put the lag report in the database... can't think of anything else to do with it.

    uint32 lagType, mapId;
    float x, y, z;

    recvData >> lagType >> mapId >> x >> y >> z;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_LAG_REPORT);
    stmt->setUInt32(0, GUID_LOPART(GetPlayer()->GetGUID()));
    stmt->setUInt8 (1, lagType);
    stmt->setUInt16(2, mapId);
    stmt->setFloat (3, x);
    stmt->setFloat (4, y);
    stmt->setFloat (5, z);
    stmt->setUInt32(6, GetLatency());
    stmt->setUInt32(7, time(NULL));
    CharacterDatabase.Execute(stmt);
}

void WorldSession::HandleGMResponseResolve(WorldPacket& /*recvPacket*/)
{
    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
    {
        sTicketMgr->CloseTicket(ticket->GetId(), GetPlayer()->GetGUID());

        sTicketMgr->SendTicketStatusUpdate(this, GMTICKET_RESPONSE_TICKET_DELETED);
        sTicketMgr->SendTicket(this, NULL);

        sTicketMgr->RemoveTicket(ticket->GetId());
        sTicketMgr->UpdateLastChange();
    }
}
