// Copyright (c) 2015-2016, Robert Escriva, Cornell University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Consus nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#define __STDC_LIMIT_MACROS

// consus
#include "test/th.h"
#include "txman/generalized_paxos.h"

using namespace consus;

struct no_conflict_comparator : public generalized_paxos::comparator
{
    no_conflict_comparator() {}
    virtual ~no_conflict_comparator() throw () {}

    virtual bool conflict(const generalized_paxos::command&, const generalized_paxos::command&) const
    {
        return false;
    }
};
no_conflict_comparator ncc;

struct all_conflict_comparator : public generalized_paxos::comparator
{
    all_conflict_comparator() {}
    virtual ~all_conflict_comparator() throw () {}

    virtual bool conflict(const generalized_paxos::command&, const generalized_paxos::command&) const
    {
        return true;
    }
};
all_conflict_comparator acc;

abstract_id _ids[] = {abstract_id(1),
                      abstract_id(2),
                      abstract_id(3),
                      abstract_id(4),
                      abstract_id(5),
                      abstract_id(6),
                      abstract_id(7),
                      abstract_id(8),
                      abstract_id(9)};

TEST(GeneralizedPaxos, FiveAcceptors)
{
    // ignore gp[0] to make 1-indexed for easy reading
    generalized_paxos gp[6];

    gp[1].init(&ncc, abstract_id(1), _ids, 5);
    gp[2].init(&ncc, abstract_id(2), _ids, 5);
    gp[3].init(&ncc, abstract_id(3), _ids, 5);
    gp[4].init(&ncc, abstract_id(4), _ids, 5);
    gp[5].init(&ncc, abstract_id(5), _ids, 5);

    bool send_m1 = false;
    bool send_m2 = false;
    bool send_m3 = false;
    generalized_paxos::message_p1a m1;
    generalized_paxos::message_p2a m2;
    generalized_paxos::message_p2b m3;
    generalized_paxos::message_p1b r1;

    gp[1].advance(true, &send_m1, &m1, &send_m2, &m2, &send_m3, &m3);
    ASSERT_TRUE(send_m1);
    ASSERT_FALSE(send_m2);
    ASSERT_FALSE(send_m3);

    for (int i = 1; i <= 5; ++i)
    {
        bool send = false;
        gp[i].process_p1a(m1, &send, &r1);

        if (send)
        {
            gp[1].process_p1b(r1);
        }
    }

    gp[1].propose(generalized_paxos::command(1, "hello world from 1"));
    gp[2].propose(generalized_paxos::command(2, "hello world from 2"));
    gp[3].propose(generalized_paxos::command(3, "hello world from 3"));
    gp[4].propose(generalized_paxos::command(4, "hello world from 4"));
    gp[5].propose(generalized_paxos::command(5, "hello world from 5"));

    gp[1].advance(true, &send_m1, &m1, &send_m2, &m2, &send_m3, &m3);
    ASSERT_FALSE(send_m1);
    ASSERT_FALSE(send_m2);
    ASSERT_TRUE(send_m3);

    for (int i = 1; i <= 5; ++i)
    {
        gp[i].process_p2b(m3);
        gp[i].propose_from_p2b(m3);
    }

    for (int i = 0; i < 10; ++i)
    {
        const unsigned idx = (i % 5) + 1;
        gp[idx].advance(false, &send_m1, &m1, &send_m2, &m2, &send_m3, &m3);
        ASSERT_FALSE(send_m1);
        ASSERT_FALSE(send_m2);
        ASSERT_TRUE(send_m3);

        for (int j = 1; j <= 5; ++j)
        {
            gp[j].process_p2b(m3);
            gp[j].propose_from_p2b(m3);
        }
    }

    for (int i = 1; i <= 5; ++i)
    {
        generalized_paxos::cstruct v = gp[i].learned();
        ASSERT_EQ(5U, v.commands.size());
    }
}

TEST(GeneralizedPaxos, Conflict)
{
    // ignore gp[0] to make 1-indexed for easy reading
    generalized_paxos gp[4];

    gp[1].init(&acc, abstract_id(1), _ids, 3);
    gp[2].init(&acc, abstract_id(2), _ids, 3);
    gp[3].init(&acc, abstract_id(3), _ids, 3);

    bool send_m1 = false;
    bool send_m2 = false;
    bool send_m3 = false;
    generalized_paxos::message_p1a m1;
    generalized_paxos::message_p2a m2;
    generalized_paxos::message_p2b m3;
    generalized_paxos::message_p1b r1;

    gp[1].advance(true, &send_m1, &m1, &send_m2, &m2, &send_m3, &m3);
    ASSERT_TRUE(send_m1);
    ASSERT_FALSE(send_m2);
    ASSERT_FALSE(send_m3);

    for (int i = 1; i <= 3; ++i)
    {
        bool send = false;
        gp[i].process_p1a(m1, &send, &r1);

        if (send)
        {
            gp[1].process_p1b(r1);
        }
    }

    gp[1].propose(generalized_paxos::command(1, "operation 1"));
    gp[1].propose(generalized_paxos::command(2, "operation 2"));
    gp[1].propose(generalized_paxos::command(3, "operation 3"));

    gp[2].propose(generalized_paxos::command(2, "operation 2"));
    gp[2].propose(generalized_paxos::command(3, "operation 3"));
    gp[2].propose(generalized_paxos::command(1, "operation 1"));

    gp[3].propose(generalized_paxos::command(3, "operation 3"));
    gp[3].propose(generalized_paxos::command(1, "operation 1"));
    gp[3].propose(generalized_paxos::command(2, "operation 2"));

    gp[1].advance(true, &send_m1, &m1, &send_m2, &m2, &send_m3, &m3);
    ASSERT_FALSE(send_m1);
    ASSERT_FALSE(send_m2);
    ASSERT_TRUE(send_m3);

    for (int i = 1; i <= 3; ++i)
    {
        gp[i].process_p2b(m3);
    }

    for (int i = 0; i < 6; ++i)
    {
        const unsigned idx = (i % 3) + 1;
        gp[idx].advance(false, &send_m1, &m1, &send_m2, &m2, &send_m3, &m3);
        ASSERT_FALSE(send_m1);
        ASSERT_FALSE(send_m1);
        ASSERT_FALSE(send_m2);
        ASSERT_TRUE(send_m3);

        for (int j = 1; j <= 3; ++j)
        {
            gp[j].process_p2b(m3);
        }
    }

    generalized_paxos::cstruct v;

    for (int i = 1; i <= 3; ++i)
    {
        v = gp[i].learned();
        ASSERT_EQ(0U, v.commands.size());
    }

    gp[1].advance(true, &send_m1, &m1, &send_m2, &m2, &send_m3, &m3);
    ASSERT_TRUE(send_m1);
    ASSERT_EQ(generalized_paxos::ballot::CLASSIC, m1.b.type);
    ASSERT_FALSE(send_m2);
    ASSERT_TRUE(send_m3);

    for (int i = 1; i <= 3; ++i)
    {
        bool send = false;
        gp[i].process_p1a(m1, &send, &r1);

        if (send)
        {
            gp[1].process_p1b(r1);
        }
    }

    gp[1].advance(true, &send_m1, &m1, &send_m2, &m2, &send_m3, &m3);
    ASSERT_FALSE(send_m1);
    ASSERT_TRUE(send_m2);
    ASSERT_TRUE(send_m3);

    for (int i = 1; i <= 3; ++i)
    {
        bool send = false;
        gp[i].process_p2a(m2, &send, &m3);

        if (send)
        {
            for (int j = 1; j <= 3; ++j)
            {
                gp[j].process_p2b(m3);
            }
        }
    }

    for (int i = 1; i <= 3; ++i)
    {
        v = gp[i].learned();
        ASSERT_EQ(3U, v.commands.size());
        generalized_paxos::cstruct u;
        u.commands.push_back(generalized_paxos::command(1, "operation 1"));
        u.commands.push_back(generalized_paxos::command(2, "operation 2"));
        u.commands.push_back(generalized_paxos::command(3, "operation 3"));
        ASSERT_EQ(v, u);
    }

    gp[1].advance(true, &send_m1, &m1, &send_m2, &m2, &send_m3, &m3);
    ASSERT_TRUE(send_m1);
    ASSERT_EQ(generalized_paxos::ballot::FAST, m1.b.type);
    ASSERT_FALSE(send_m2);
    ASSERT_TRUE(send_m3);
}

generalized_paxos::cstruct
cons(generalized_paxos::cstruct cs, generalized_paxos::command c)
{
    cs.commands.push_back(c);
    return cs;
}

TEST(GeneralizedPaxos, Bug1)
{
bool throwaway_bool;
generalized_paxos::message_p1a throwaway_p1a;
generalized_paxos::message_p1b throwaway_p1b;
generalized_paxos::message_p2a throwaway_p2a;
generalized_paxos::message_p2b throwaway_p2b;
abstract_id ids[5];

for (unsigned i = 0; i < 5; ++i)
{
    ids[i] = abstract_id(i + 1);
}

generalized_paxos gp;
gp.init(&acc, ids[1], &ids[0], 5);
// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1a(b=ballot(type=FAST, number=1, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1a(b=ballot(type=CLASSIC, number=2, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(2), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(3), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}
}


TEST(GeneralizedPaxos, Bug2)
{
// =============================================================================
// abstract(4) errored out
bool throwaway_bool;
generalized_paxos::message_p1a throwaway_p1a;
generalized_paxos::message_p1b throwaway_p1b;
generalized_paxos::message_p2a throwaway_p2a;
generalized_paxos::message_p2b throwaway_p2b;
abstract_id ids[5];

for (unsigned i = 0; i < 5; ++i)
{
    ids[i] = abstract_id(i + 1);
}

generalized_paxos gp;
gp.init(&acc, ids[3], &ids[0], 5);
// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1a(b=ballot(type=FAST, number=1, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1a(b=ballot(type=CLASSIC, number=2, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(2), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(3), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}
}


TEST(GeneralizedPaxos, Bug3)
{
// =============================================================================
// abstract(3) errored out
bool throwaway_bool;
generalized_paxos::message_p1a throwaway_p1a;
generalized_paxos::message_p1b throwaway_p1b;
generalized_paxos::message_p2a throwaway_p2a;
generalized_paxos::message_p2b throwaway_p2b;
abstract_id ids[5];

for (unsigned i = 0; i < 5; ++i)
{
    ids[i] = abstract_id(i + 1);
}

generalized_paxos gp;
gp.init(&acc, ids[2], &ids[0], 5);
// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1a(b=ballot(type=FAST, number=1, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1a(b=ballot(type=CLASSIC, number=2, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(2), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(3), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}
}


TEST(GeneralizedPaxos, Bug4)
{
// =============================================================================
// abstract(1) errored out
bool throwaway_bool;
generalized_paxos::message_p1a throwaway_p1a;
generalized_paxos::message_p1b throwaway_p1b;
generalized_paxos::message_p2a throwaway_p2a;
generalized_paxos::message_p2b throwaway_p2b;
abstract_id ids[5];

for (unsigned i = 0; i < 5; ++i)
{
    ids[i] = abstract_id(i + 1);
}

generalized_paxos gp;
gp.init(&acc, ids[0], &ids[0], 5);
// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)));
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1a(b=ballot(type=FAST, number=1, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)));
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)));
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)));
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1a(b=ballot(type=CLASSIC, number=2, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(2), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(3), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(true, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}
}


TEST(GeneralizedPaxos, Bug5)
{
// =============================================================================
// abstract(5) errored out
bool throwaway_bool;
generalized_paxos::message_p1a throwaway_p1a;
generalized_paxos::message_p1b throwaway_p1b;
generalized_paxos::message_p2a throwaway_p2a;
generalized_paxos::message_p2b throwaway_p2b;
abstract_id ids[5];

for (unsigned i = 0; i < 5; ++i)
{
    ids[i] = abstract_id(i + 1);
}

generalized_paxos gp;
gp.init(&acc, ids[4], &ids[0], 5);
// message_p1a(b=ballot(type=FAST, number=1, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(2), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(2), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), vb=ballot(type=CLASSIC, number=0, leader=0), v=cstruct([])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), generalized_paxos::ballot(), generalized_paxos::cstruct()))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(3), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(3), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")
gp.propose(generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)));
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(4), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(4), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(5), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(5), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1a(b=ballot(type=CLASSIC, number=2, leader=1))
gp.process_p1a(generalized_paxos::message_p1a(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1))), &throwaway_bool, &throwaway_p1b);
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(2), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(2), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p1b(b=ballot(type=CLASSIC, number=2, leader=1), acceptor=abstract(3), vb=ballot(type=FAST, number=1, leader=1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p1b(generalized_paxos::message_p1b(generalized_paxos::ballot(generalized_paxos::ballot::CLASSIC, 2, abstract_id(1)), abstract_id(3), generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}

// message_p2b(b=ballot(type=FAST, number=1, leader=1), acceptor=abstract(1), v=cstruct([command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x00"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x01"), command(type=1, value="\x00\x00\x00\x00\x00\x00\x00\x02")])
if (gp.process_p2b(generalized_paxos::message_p2b(generalized_paxos::ballot(generalized_paxos::ballot::FAST, 1, abstract_id(1)), abstract_id(1), cons(cons(cons(generalized_paxos::cstruct(), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x00", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8))), generalized_paxos::command(1, std::string("\x00\x00\x00\x00\x00\x00\x00\x02", 8)))))) {
gp.advance(false, &throwaway_bool, &throwaway_p1a, &throwaway_bool, &throwaway_p2a, &throwaway_bool, &throwaway_p2b);
gp.learned();
}
}
