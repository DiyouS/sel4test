/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/capops.h>
#include "../helpers.h"

#define MIN_LENGTH 0
#define MAX_LENGTH (seL4_MsgMaxLength + 1)

#define FOR_EACH_LENGTH(len_var) \
    for(int len_var = MIN_LENGTH; len_var <= MAX_LENGTH; len_var++)

typedef int (*test_func_t)(seL4_Word /* endpoint */, seL4_Word /* seed */, seL4_Word /* extra */);

static int
send_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, length);
        for (int i = 0; i < length; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }
        seL4_Send(endpoint, tag);
    }

    return sel4test_get_result();
}

static int
nbsend_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, length);
        for (int i = 0; i < length; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }
        seL4_NBSend(endpoint, tag);
    }

    return sel4test_get_result();
}

static int
call_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, length);

        /* Construct a message. */
        for (int i = 0; i < length; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }

        tag = seL4_Call(endpoint, tag);

        seL4_Word actual_len = length;
        /* Sanity check the received message. */
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
    }

    return sel4test_get_result();
}

static int
wait_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag;
        seL4_Word sender_badge = 0;

        tag = seL4_Recv(endpoint, &sender_badge);
        seL4_Word actual_len = length;
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
    }

    return sel4test_get_result();
}

static int
nbwait_func(seL4_Word endpoint, seL4_Word seed, seL4_Word nbwait_should_wait)
{
    if (!nbwait_should_wait) {
        return sel4test_get_result();
    }

    FOR_EACH_LENGTH(length) {
        seL4_MessageInfo_t tag;
        seL4_Word sender_badge = 0;

        tag = seL4_Recv(endpoint, &sender_badge);
        seL4_Word actual_len = length;
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
    }

    return sel4test_get_result();
}

static int
replywait_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    int first = 1;

    seL4_MessageInfo_t tag;
    FOR_EACH_LENGTH(length) {
        seL4_Word sender_badge = 0;

        /* First reply/wait can't reply. */
        if (first) {
            tag = seL4_Recv(endpoint, &sender_badge);
            first = 0;
        } else {
            tag = seL4_ReplyRecv(endpoint, tag, &sender_badge);
        }

        seL4_Word actual_len = length;
        /* Sanity check the received message. */
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
        /* Seed will have changed more if the message was truncated. */
        for (int i = actual_len; i < length; i++) {
            seed++;
        }

        /* Construct a reply. */
        for (int i = 0; i < actual_len; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }
    }

    /* Need to do one last reply to match call. */
    seL4_Reply(tag);

    return sel4test_get_result();
}

static int
reply_and_wait_func(seL4_Word endpoint, seL4_Word seed, seL4_Word arg2)
{
    int first = 1;

    seL4_MessageInfo_t tag;
    FOR_EACH_LENGTH(length) {
        seL4_Word sender_badge = 0;

        /* First reply/wait can't reply. */
        if (!first) {
            seL4_Reply(tag);
        } else {
            first = 0;
        }

        tag = seL4_Recv(endpoint, &sender_badge);

        seL4_Word actual_len = length;
        /* Sanity check the received message. */
        if (actual_len <= seL4_MsgMaxLength) {
            test_assert(seL4_MessageInfo_get_length(tag) == actual_len);
        } else {
            actual_len = seL4_MsgMaxLength;
        }

        for (int i = 0; i < actual_len; i++) {
            seL4_Word mr = seL4_GetMR(i);
            test_check(mr == seed);
            seed++;
        }
        /* Seed will have changed more if the message was truncated. */
        for (int i = actual_len; i < length; i++) {
            seed++;
        }

        /* Construct a reply. */
        for (int i = 0; i < actual_len; i++) {
            seL4_SetMR(i, seed);
            seed++;
        }
    }

    /* Need to do one last reply to match call. */
    seL4_Reply(tag);

    return sel4test_get_result();
}

static int
test_ipc_pair(env_t env, test_func_t fa, test_func_t fb, bool inter_as)
{
    helper_thread_t thread_a, thread_b;
    vka_t *vka = &env->vka;

    int error;
    seL4_CPtr ep = vka_alloc_endpoint_leaky(vka);
    seL4_Word start_number = 0xabbacafe;

    /* Test sending messages of varying lengths. */
    /* Please excuse the awful indending here. */
    for (int sender_prio = 98; sender_prio <= 102; sender_prio++) {
        for (int waiter_prio = 100; waiter_prio <= 100; waiter_prio++) {
            for (int sender_first = 0; sender_first <= 1; sender_first++) {
                ZF_LOGD("%d %s %d\n",
                        sender_prio, sender_first ? "->" : "<-", waiter_prio);
                seL4_Word thread_a_arg0, thread_b_arg0;

                if (inter_as) {
                    create_helper_process(env, &thread_a);

                    cspacepath_t path;
                    vka_cspace_make_path(&env->vka, ep, &path);
                    thread_a_arg0 = sel4utils_copy_cap_to_process(&thread_a.process, path);
                    assert(thread_a_arg0 != -1);

                    create_helper_process(env, &thread_b);
                    thread_b_arg0 = sel4utils_copy_cap_to_process(&thread_b.process, path);
                    assert(thread_b_arg0 != -1);

                } else {
                    create_helper_thread(env, &thread_a);
                    create_helper_thread(env, &thread_b);
                    thread_a_arg0 = ep;
                    thread_b_arg0 = ep;
                }

                set_helper_priority(&thread_a, sender_prio);
                set_helper_priority(&thread_b, waiter_prio);

                /* Set the flag for nbwait_func that tells it whether or not it really
                 * should wait. */
                int nbwait_should_wait;
                nbwait_should_wait =
                    (sender_prio < waiter_prio);

                /* Threads are enqueued at the head of the scheduling queue, so the
                 * thread enqueued last will be run first, for a given priority. */
                if (sender_first) {
                    start_helper(env, &thread_b, (helper_fn_t) fb, thread_b_arg0, start_number,
                                 nbwait_should_wait, 0);
                    start_helper(env, &thread_a, (helper_fn_t) fa, thread_a_arg0, start_number,
                                 nbwait_should_wait, 0);
                } else {
                    start_helper(env, &thread_a, (helper_fn_t) fa, thread_a_arg0, start_number,
                                 nbwait_should_wait, 0);
                    start_helper(env, &thread_b, (helper_fn_t) fb, thread_b_arg0, start_number,
                                 nbwait_should_wait, 0);
                }

                wait_for_helper(&thread_a);
                wait_for_helper(&thread_b);

                cleanup_helper(env, &thread_a);
                cleanup_helper(env, &thread_b);

                start_number += 0x71717171;
            }
        }
    }

    error = cnode_delete(env, ep);
    test_assert(!error);
    return sel4test_get_result();
}

static int
test_send_wait(env_t env)
{
    return test_ipc_pair(env, send_func, wait_func, false);
}
DEFINE_TEST(IPC0001, "Test seL4_Send + seL4_Recv", test_send_wait)

static int
test_call_replywait(env_t env)
{
    return test_ipc_pair(env, call_func, replywait_func, false);
}
DEFINE_TEST(IPC0002, "Test seL4_Call + seL4_ReplyRecv", test_call_replywait)

static int
test_call_reply_and_wait(env_t env)
{
    return test_ipc_pair(env, call_func, reply_and_wait_func, false);
}
DEFINE_TEST(IPC0003, "Test seL4_Send + seL4_Reply + seL4_Recv", test_call_reply_and_wait)

static int
test_nbsend_wait(env_t env)
{
    return test_ipc_pair(env, nbsend_func, nbwait_func, false);
}
DEFINE_TEST(IPC0004, "Test seL4_NBSend + seL4_Recv", test_nbsend_wait)

static int
test_send_wait_interas(env_t env)
{
    return test_ipc_pair(env, send_func, wait_func, true);
}
DEFINE_TEST(IPC1001, "Test inter-AS seL4_Send + seL4_Recv", test_send_wait_interas)

static int
test_call_replywait_interas(env_t env)
{
    return test_ipc_pair(env, call_func, replywait_func, true);
}
DEFINE_TEST(IPC1002, "Test inter-AS seL4_Call + seL4_ReplyRecv", test_call_replywait_interas)

static int
test_call_reply_and_wait_interas(env_t env)
{
    return test_ipc_pair(env, call_func, reply_and_wait_func, true);
}
DEFINE_TEST(IPC1003, "Test inter-AS seL4_Send + seL4_Reply + seL4_Recv", test_call_reply_and_wait_interas)

static int
test_nbsend_wait_interas(env_t env)
{
    return test_ipc_pair(env, nbsend_func, nbwait_func, true);
}
DEFINE_TEST(IPC1004, "Test inter-AS seL4_NBSend + seL4_Recv", test_nbsend_wait_interas)

static int
test_ipc_abort_in_call(env_t env)
{
    helper_thread_t thread_a;
    vka_t * vka = &env->vka;

    seL4_CPtr ep = vka_alloc_endpoint_leaky(vka);

    seL4_Word start_number = 0xabbacafe;

    create_helper_thread(env, &thread_a);
    set_helper_priority(&thread_a, 100);

    start_helper(env, &thread_a, (helper_fn_t) call_func, ep, start_number, 0, 0);

    /* Wait for the endpoint that it's going to call. */
    seL4_Word sender_badge = 0;

    seL4_Recv(ep, &sender_badge);

    /* Now suspend the thread. */
    seL4_TCB_Suspend(thread_a.thread.tcb.cptr);

    /* Now resume the thread. */
    seL4_TCB_Resume(thread_a.thread.tcb.cptr);

    /* Now suspend it again for good measure. */
    seL4_TCB_Suspend(thread_a.thread.tcb.cptr);

    /* And delete it. */
    cleanup_helper(env, &thread_a);

    return sel4test_get_result();
}
DEFINE_TEST(IPC0010, "Test suspending an IPC mid-Call()", test_ipc_abort_in_call)

static void
server_fn(seL4_CPtr endpoint, int runs, volatile int *state)
{

    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 0);

    /* signal the intialiser that we are done */
    ZF_LOGD("Server call");
    *state = *state + 1;
    seL4_NBSendRecv(endpoint, info, endpoint, NULL);
    /* from here on we are running on borrowed time */

    int i = 0;
    while (i < runs) {
        test_assert_fatal(seL4_GetMR(0) == 12345678);

        uint32_t length = seL4_GetMR(1);
        seL4_SetMR(0, 0xdeadbeef);
        info = seL4_MessageInfo_new(0, 0, 0, length);

        *state = *state + 1;
        ZF_LOGD("Server replyWait\n");
        seL4_ReplyRecv(endpoint, info, NULL);
        i++;
    }

}

static void
proxy_fn(seL4_CPtr receive_endpoint, seL4_CPtr call_endpoint, int runs, volatile int *state)
{
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 0);

    /* signal the initialiser that we are awake */
    ZF_LOGD("Proxy Call");
    *state = *state + 1;
    seL4_NBSendRecv(call_endpoint, info, receive_endpoint, NULL);
    /* when we get here we are running on a donated scheduling context, 
       as the initialiser has taken ours away */

    int i = 0;
    while (i < runs) {
        test_assert_fatal(seL4_GetMR(0) == 12345678);

        uint32_t length = seL4_GetMR(1);
        seL4_SetMR(0, 12345678);
        seL4_SetMR(1, length);
        info = seL4_MessageInfo_new(0, 0, 0, length);

        ZF_LOGD("Proxy call\n");
        seL4_Call(call_endpoint, info);

        test_assert_fatal(seL4_GetMR(0) == 0xdeadbeef);

        seL4_SetMR(0, 0xdeadbeef);
        ZF_LOGD("Proxy replyWait\n");
        *state = *state + 1;
        seL4_ReplyRecv(receive_endpoint, info, NULL);
        i++;
    }

}

static void
client_fn(seL4_CPtr endpoint, bool fastpath, int runs, volatile int *state)
{

    /* make the message greater than 4 in size if we do not want to hit the fastpath */
    uint32_t length = fastpath ? 2 : 5;

    int i = 0;
    while (i < runs) {
        seL4_SetMR(0, 12345678);
        seL4_SetMR(1, length);
        seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, length);

        ZF_LOGD("Client call\n");
        info = seL4_Call(endpoint, info);

        test_assert_fatal(seL4_GetMR(0) == 0xdeadbeef);
        i++;
        *state = *state + 1;
    }
}

static int
single_client_server_chain_test(env_t env, int fastpath, int prio_diff)
{
    int error;
    const int runs = 10;
    const int num_proxies = 5;
    int client_prio = 10;
    int server_prio = client_prio + (prio_diff * num_proxies);
    helper_thread_t client, server;
    helper_thread_t proxies[num_proxies];
    volatile int client_state = 0;
    volatile int server_state = 0;
    volatile int proxy_state[num_proxies];


    /* create client */
    create_helper_thread(env, &client);
    set_helper_sched_params(env, &client, 1000llu, 1000llu);
    set_helper_priority(&client, client_prio);

    
    seL4_CPtr receive_endpoint = vka_alloc_endpoint_leaky(&env->vka);
    seL4_CPtr first_endpoint = receive_endpoint;
    /* create proxies */
    for (int i = 0; i < num_proxies; i++) {
        int prio = server_prio + (prio_diff * i);
        proxy_state[i] = 0;
        seL4_CPtr call_endpoint = vka_alloc_endpoint_leaky(&env->vka);
        create_helper_thread(env, &proxies[i]);
        set_helper_priority(&proxies[i], prio);
        ZF_LOGD("Start proxy\n");
        start_helper(env, &proxies[i], (helper_fn_t) proxy_fn, receive_endpoint,
                     call_endpoint, runs, (seL4_Word) &proxy_state[i]);
        
        /* wait for proxy to initialise */
        ZF_LOGD("Wait for proxy\n");
        seL4_Recv(call_endpoint, NULL);
        test_eq(proxy_state[i], 1);
        /* now take away its scheduling context */
        error = seL4_SchedContext_UnbindTCB(proxies[i].thread.sched_context.cptr);
        test_eq(error, seL4_NoError);

        receive_endpoint = call_endpoint;
    }

    /* create the server */
    create_helper_thread(env, &server);
    set_helper_priority(&server, server_prio); 
    ZF_LOGD("Start server");
    start_helper(env, &server, (helper_fn_t) server_fn, receive_endpoint, runs, 
                 (seL4_Word) &server_state, 0);
    /* wait for server to initialise on our time */
    ZF_LOGD("Wait for server");
    seL4_Recv(receive_endpoint, NULL);
    test_eq(server_state, 1);
    /* now take it's scheduling context away */
    error = seL4_SchedContext_UnbindTCB(server.thread.sched_context.cptr);
    test_eq(error, seL4_NoError);

    ZF_LOGD("Start client");
    start_helper(env, &client, (helper_fn_t) client_fn, first_endpoint, 
                 fastpath, runs, (seL4_Word) &client_state); 
    
    /* sleep and let the testrun */
    ZF_LOGD("Wait for client");
    wait_for_helper(&client);

    test_eq(server_state, runs + 1);
    test_eq(client_state, runs);
    for (int i = 0; i < num_proxies; i++) {
        test_eq(proxy_state[i], runs + 1);
    }

    return sel4test_get_result();
}

int
test_single_client_slowpath_same_prio(env_t env)
{
    return single_client_server_chain_test(env, 0, 0);
}
DEFINE_TEST(IPC0011, "Client-server inheritance: slowpath, same prio", test_single_client_slowpath_same_prio)

int
test_single_client_slowpath_higher_prio(env_t env)
{
    return single_client_server_chain_test(env, 0, 1);
}
DEFINE_TEST(IPC0012, "Client-server inheritance: slowpath, client higher prio", test_single_client_slowpath_higher_prio)

int
test_single_client_slowpath_lower_prio(env_t env)
{
    return single_client_server_chain_test(env, 0, -1);
}
DEFINE_TEST(IPC0013, "Client-server inheritance: slowpath, client lower prio", test_single_client_slowpath_lower_prio)

int
test_single_client_fastpath_higher_prio(env_t env)
{
    return single_client_server_chain_test(env, 1, 1);
}
DEFINE_TEST(IPC0014, "Client-server inheritance: fastpath, client higher prio", test_single_client_fastpath_higher_prio)

int
test_single_client_fastpath_same_prio(env_t env)
{
    return single_client_server_chain_test(env, 1, 0);
}
DEFINE_TEST(IPC0015, "Client-server inheritance: fastpath, client same prio", test_single_client_fastpath_same_prio)

static void
ipc0016_call_once_fn(seL4_CPtr endpoint, volatile int *state)
{
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 0);
  
    *state = *state + 1;
    ZF_LOGD("Call %d\n", *state);
    seL4_Call(endpoint, info);
    ZF_LOGD("Resumed with reply\n");
    *state = *state + 1;
 
}

static void
ipc0016_reply_once_fn(seL4_CPtr endpoint)
{
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 0);
  
    /* send initialisation context back */
    ZF_LOGD("seL4_NBSendRecv\n");
    seL4_NBSendRecv(endpoint, info, endpoint, NULL);
  
    /* reply */
    ZF_LOGD("Reply\n");
    seL4_Reply(info);
      
    /* wait (keeping sc) */
    ZF_LOGD("Wait\n");
    seL4_Wait(endpoint, NULL);

    test_assert_fatal(!"should not get here");
}

static int
test_transfer_on_reply(env_t env)
{
    volatile int state = 1;
    int error;
    helper_thread_t client, server;
    seL4_CPtr endpoint;

    endpoint = vka_alloc_endpoint_leaky(&env->vka);
    create_helper_thread(env, &client); 
    create_helper_thread(env, &server);

    set_helper_priority(&client, 10);
    set_helper_priority(&server, 11);

    start_helper(env, &server, (helper_fn_t) ipc0016_reply_once_fn, endpoint, 0, 0, 0);

    /* wait for server to initialise */
    seL4_Recv(endpoint, NULL);
    /* now remove the schedluing context */
    error = seL4_SchedContext_UnbindTCB(server.thread.sched_context.cptr);
    test_eq(error, seL4_NoError);

    /* start the client */
    start_helper(env, &client, (helper_fn_t) ipc0016_call_once_fn, endpoint, 
                 (seL4_Word) &state, 0, 0);
    
    /* the server will attempt to steal the clients scheduling context 
     * by using seL4_Reply instead of seL4_ReplyWait. However, 
     * a reply cap is a guarantee that a scheduling context will be returned,
     * so it does return to the client and the server hangs.
     */
    wait_for_helper(&client);

    test_eq(state, 3);

    return sel4test_get_result();
}
DEFINE_TEST(IPC0016, "Test reply does returns scheduling scheduling context", 
        test_transfer_on_reply);

/* used by ipc0017 and ipc0019 */
static void
sender(seL4_CPtr endpoint, volatile int *state)
{
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 0);
    ZF_LOGD("Client send\n");
    *state = 1;
    seL4_Send(endpoint, info);
    *state = 2;
}

static void
wait_server(seL4_CPtr endpoint, int runs)
{
    /* signal test that we are initialised */
    seL4_Send(endpoint, seL4_MessageInfo_new(0, 0, 0, 0));

    int i = 0;
    while (i < runs) {
        ZF_LOGD("Server wait\n");
        seL4_Recv(endpoint, NULL);
        i++;
    }
}

static int
test_send_to_no_sc(env_t env)
{
    /* sends should block until the server gets a scheduling context.
     * nb sends should not block */
    int error;
    helper_thread_t server, client1, client2;
    volatile int state1, state2;
    seL4_CPtr endpoint;

    endpoint = vka_alloc_endpoint_leaky(&env->vka);

    create_helper_thread(env, &server);
    create_helper_thread(env, &client1);
    create_helper_thread(env, &client2);

    set_helper_priority(&server, 10);
    set_helper_priority(&client1, 9);
    set_helper_priority(&client2, 9);
    
    const int num_server_messages = 4;
    start_helper(env, &server, (helper_fn_t) wait_server, endpoint, num_server_messages, 0, 0);
    seL4_Recv(endpoint, NULL);

    error = seL4_SchedContext_UnbindTCB(server.thread.sched_context.cptr);
    test_eq(error, seL4_NoError);

    /* this message should not result in the server being scheduled */
    ZF_LOGD("NBSend");
    seL4_SetMR(0, 12345678);
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_NBSend(endpoint, info);
    test_eq(seL4_GetMR(0), 12345678);

    /* start clients */
    state1 = 0;
    state2 = 0;
    start_helper(env, &client1, (helper_fn_t) sender, endpoint, (seL4_Word) &state1, 0, 0);
    start_helper(env, &client2, (helper_fn_t) sender, endpoint, (seL4_Word) &state2, 0 , 0);
    
    /* set our prio down, both clients should block as the server cannot 
     * run without a schedluing context */
    error = seL4_TCB_SetPriority(env->tcb, 8);
    test_eq(error, seL4_NoError);
    test_eq(state1, 1);
    test_eq(state2, 1);

    /* restore the servers schedluing context */
    error = seL4_SchedContext_BindTCB(server.thread.sched_context.cptr, server.thread.tcb.cptr);
    test_eq(error, seL4_NoError);

   /* now the clients should be unblocked */
    test_eq(state1, 2);
    test_eq(state2, 2);

    /* this should work */
    seL4_NBSend(endpoint, info);
   
    /* and so should this */
    seL4_Send(endpoint, info);

    /* if the server received the correct number of messages it should now be done */
    wait_for_helper(&server);

    return sel4test_get_result();
}
DEFINE_TEST(IPC0017, "Test seL4_Send/seL4_NBSend to a server with no scheduling context", test_send_to_no_sc)

static void
ipc0018_helper(seL4_CPtr endpoint, volatile int *state) 
{
    *state = 1;

    while (1) {
        ZF_LOGD("Send");
        seL4_Send(endpoint, seL4_MessageInfo_new(0, 0, 0, 0));
        *state = *state + 1;
    }
}

static int
test_receive_no_sc(env_t env) 
{
    helper_thread_t client;
    int error; 
    volatile int state;
    seL4_CPtr endpoint;

    endpoint = vka_alloc_endpoint_leaky(&env->vka);
    create_helper_thread(env, &client);
    set_helper_priority(&client, 10);
    error = seL4_TCB_SetPriority(env->tcb, 9);
    test_eq(error, seL4_NoError);

    /* start the client, it will increment state and send a message */
    start_helper(env, &client, (helper_fn_t) ipc0018_helper, endpoint, 
                 (seL4_Word) &state, 0, 0);

    test_eq(state, 1);

    /* clear the clients scheduling context */
    ZF_LOGD("Unbind scheduling context");
    error = seL4_SchedContext_UnbindTCB(client.thread.sched_context.cptr);
    test_eq(error, seL4_NoError);

    /* now we should be able to receive the message, but since the client
     * no longer has a schedluing context it should not run 
     */
    ZF_LOGD("Recv");
    seL4_Recv(endpoint, NULL);
    
    /* check thread has not run */
    test_eq(state, 1);

    /* now set the schedluing context again */
    error = seL4_SchedContext_BindTCB(client.thread.sched_context.cptr, 
                                      client.thread.tcb.cptr);
    test_eq(error, seL4_NoError);
    test_eq(state, 2);

    /* now get another message */
    seL4_Recv(endpoint, NULL);
    test_eq(state, 3);

    /* and another, to check client is well and truly running */
    seL4_Recv(endpoint, NULL);
    test_eq(state, 4);

    return sel4test_get_result();
}   
DEFINE_TEST(IPC0018, "Test receive from a client with no scheduling context", 
            test_receive_no_sc);

static int 
delete_sc_client_sending_on_endpoint(env_t env) 
{
    helper_thread_t client;
    seL4_CPtr endpoint;
    volatile int state = 0;
    int error;

    endpoint = vka_alloc_endpoint_leaky(&env->vka);

    create_helper_thread(env, &client);
    set_helper_priority(&client, 10);
    
    /* set our prio below the helper */
    error = seL4_TCB_SetPriority(env->tcb, 9);
    test_eq(error, seL4_NoError);

    start_helper(env, &client, (helper_fn_t) sender, endpoint, (seL4_Word) &state, 0, 0);
       
    /* the client will run and send on the endpoint */
    test_eq(state, 1);

    /* now delete the scheduling context - this should unbind the client 
     * but not remove the message */

    ZF_LOGD("Destroying schedluing context");
    vka_free_object(&env->vka, &client.thread.sched_context);

    ZF_LOGD("seL4_Recv");
    seL4_Recv(endpoint, NULL);
    test_eq(state, 1);

    return sel4test_get_result();
}
DEFINE_TEST(IPC0019, "Test deleteing the scheduling context while a client is sending on an endpoint", delete_sc_client_sending_on_endpoint);

static void
ipc0020_helper(seL4_CPtr endpoint, volatile int *state)
{
    *state = 1;
    while (1) {
        ZF_LOGD("Recv");
        seL4_Recv(endpoint, NULL);
        *state = *state + 1;
    }
}

static int
delete_sc_client_waiting_on_endpoint(env_t env)
{
    helper_thread_t waiter;
    volatile int state = 0;
    int error;
    seL4_CPtr endpoint = vka_alloc_endpoint_leaky(&env->vka);

    create_helper_thread(env, &waiter);
    set_helper_priority(&waiter, 10);
    error = seL4_TCB_SetPriority(env->tcb, 9);
    start_helper(env, &waiter, (helper_fn_t) ipc0020_helper, endpoint, (seL4_Word) &state, 0, 0);

    /* helper should run and block receiving on endpoint */
    test_eq(state, 1);
 
    /* destroy scheduling context */
    vka_free_object(&env->vka, &waiter.thread.sched_context);

    /* send message */
    seL4_Send(endpoint, seL4_MessageInfo_new(0, 0, 0, 0));
    
    /* thread should not have moved */
    test_eq(state, 1);

    /* now create a new scheduling context and give it to the thread */
    seL4_CPtr sched_context = vka_alloc_sched_context_leaky(&env->vka);
    error = seL4_SchedControl_Configure(simple_get_sched_ctrl(&env->simple), sched_context,
                                        1000 * US_IN_S, 1000 * US_IN_S); 
    test_eq(error, seL4_NoError);

    error = seL4_SchedContext_BindTCB(sched_context, waiter.thread.tcb.cptr);
    test_eq(error, seL4_NoError);

    /* now the thread should run and receive the message */
    test_eq(state, 2);

    return sel4test_get_result();
}
DEFINE_TEST(IPC0020, "test deleting a scheduling context while the client is waiting on an endpoint", delete_sc_client_waiting_on_endpoint);



