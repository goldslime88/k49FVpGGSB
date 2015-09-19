#include "receiver.h"

void init_receiver(Receiver * receiver,
                   int id)
{
	pthread_cond_init(&receiver->buffer_cv, NULL);
	pthread_mutex_init(&receiver->buffer_mutex, NULL);
	receiver->recv_id = id;
    receiver->input_framelist_head = NULL;
    receiver->LAF = 7;
    receiver->LFR = -1;
    receiver->RWS = 8;
    int i;
    for(i=0;i<8;i++){
        receiver->isReceived_Frame[i] = 0;
        receiver->cached_FrameArray[i] = (char *) malloc(sizeof(char)*MAX_FRAME_SIZE);
        memset(receiver->cached_FrameArray[i],0,MAX_FRAME_SIZE);
    }
    receiver->pendingMessage = 0;
}


void handle_incoming_msgs(Receiver * receiver,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming frames
    //    1) Dequeue the Frame from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this receiver
    //    5) Do sliding window protocol for sender/receiver pair
    // fprintf(stderr, "in handle_incoming_msgs\n");

    int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
    while (incoming_msgs_length > 0)
    {
        //Pop a node off the front of the link list and update the count
        LLnode * ll_inmsg_node = ll_pop_node(&receiver->input_framelist_head);
        incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        
        //DUMMY CODE: Print the raw_char_buf
        //NOTE: You should not blindly print messages!
        //      Ask yourself: Is this message really for me?
        //                    Is this message corrupted?
        //                    Is this an old, retransmitted message?           
        char * raw_char_buf = (char *) ll_inmsg_node->value;
        Frame * inframe = convert_char_to_frame(raw_char_buf);
        uint8_t isCorrect = (uint8_t)crc8Caculate(raw_char_buf, 64);
        // fprintf(stderr, "message reminder is %d\n", isCorrect );
        //Free raw_char_buf
        free(raw_char_buf);
        uint16_t src;
        uint16_t dst;
        uint8_t seqNum; 

        src = (uint16_t)(inframe->send_id[0]) * 256 +  (uint16_t)(inframe->send_id[1]);
        dst = (uint16_t)(inframe->recv_id[0]) * 256 +  (uint16_t)(inframe->recv_id[1]);
        seqNum = (uint8_t)(inframe->seqNum[0]);
        // fprintf(stderr, "haha%d \n", strlen(inframe->data));
        if(isCorrect != 0){
            fprintf(stderr, "%u is corrupted.\n", (uint8_t)(seqNum));  
        }
        uint8_t isInWindow = 2;
        if(receiver->LFR > receiver->LAF){
            if(seqNum >receiver->LFR || seqNum <= receiver->LAF)
                isInWindow = 1;
            else if(seqNum<=receiver->LFR)
                isInWindow = 0;
        }
        else{
            if(seqNum >receiver->LFR && seqNum <= receiver->LAF){
                isInWindow = 1;
            }
            else if(seqNum<=receiver->LFR||(seqNum>=248&&seqNum<=255)){
                isInWindow = 0;
            }
        }
        uint8_t slotNum;
        slotNum = (uint8_t)(seqNum - receiver->LFR - 1);

        if((receiver->isReceived_Frame[slotNum] == 1 || isInWindow == 0)&&isCorrect==0){
            
            Frame * outgoing_frame = (Frame *) malloc (sizeof(Frame));
            outgoing_frame->recv_id[0] = (char)((src>>8) & 0xFF);
            outgoing_frame->recv_id[1] = (char)((src) & 0xFF);
            outgoing_frame->send_id[0] = (char)((dst>>8) & 0xFF);
            outgoing_frame->send_id[1] = (char)((dst) & 0xFF);
            outgoing_frame->seqNum[0] = (char)(seqNum);
            //Convert the message to the outgoing_charbuf
            char * calculating_charbuf = convert_frame_to_char(outgoing_frame);
            uint8_t crc = (uint8_t)crc8Caculate(calculating_charbuf, 63);
            outgoing_frame->crc[0] = (char)(crc);
            fprintf(stderr, "Duplicate Ack of %d sent\n", seqNum );

            char * outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            ll_append_node(outgoing_frames_head_ptr,
                               outgoing_charbuf);
            free(outgoing_frame);
            free(calculating_charbuf);
        }
        else if(dst == receiver->recv_id && isInWindow == 1 && isCorrect == 0
            && receiver->isReceived_Frame[slotNum] == 0){
            fprintf(stderr, "%u received. \n", (uint8_t)(seqNum));
            fprintf(stderr, "Ack of %d sent\n", seqNum );  
            char *tempbuf = convert_frame_to_char(inframe);
            memset(receiver->cached_FrameArray[slotNum],0,MAX_FRAME_SIZE);
            memcpy(receiver->cached_FrameArray[slotNum],tempbuf, MAX_FRAME_SIZE);
            receiver->isReceived_Frame[slotNum] = 1;
            
            uint8_t i, move;
            uint8_t record;
            if(slotNum == 0){
                move = 1;
                receiver->isReceived_Frame[0] = 0;

                Frame *tempFrame = convert_char_to_frame(receiver->cached_FrameArray[0]);
                printf("<RECV_%d>:[%s]\n", receiver->recv_id, tempFrame->data);
                record = 8;
                for(i = 1; i < 8 ; i++){
                    if(receiver->isReceived_Frame[i] == 1){
                        receiver->isReceived_Frame[i] = 0;
                        Frame *tempFrame = convert_char_to_frame(receiver->cached_FrameArray[i]);
                        printf("<RECV_%d>:[%s]\n", receiver->recv_id, tempFrame->data);
                        move ++;
                    }
                    else{
                        record = i;
                        break;
                    }
                }
                for(i = 0; i < 8-record; i++){
                    receiver->isReceived_Frame[i] = receiver->isReceived_Frame[i+record];
                    memset(receiver->cached_FrameArray[i],0,MAX_FRAME_SIZE);
                    memcpy(receiver->cached_FrameArray[i],receiver->cached_FrameArray[i+record], MAX_FRAME_SIZE);
                }
                for(i = 8-record; i<8; i++){
                    receiver->isReceived_Frame[i] = 0;
                    memset(receiver->cached_FrameArray[i],0,MAX_FRAME_SIZE);
                }
                receiver->LFR += move;
                receiver->LAF = receiver->LFR + receiver->RWS;
            }
            fprintf(stderr, "LFR is %d, LAF is %d\n",receiver->LFR,receiver->LAF);
            
            Frame * outgoing_frame = (Frame *) malloc (sizeof(Frame));
            outgoing_frame->recv_id[0] = (char)((src>>8) & 0xFF);
            outgoing_frame->recv_id[1] = (char)((src) & 0xFF);
            outgoing_frame->send_id[0] = (char)((dst>>8) & 0xFF);
            outgoing_frame->send_id[1] = (char)((dst) & 0xFF);
            outgoing_frame->seqNum[0] = (char)(seqNum);
            //Convert the message to the outgoing_charbuf
            char * calculating_charbuf = convert_frame_to_char(outgoing_frame);
            uint8_t crc = (uint8_t)crc8Caculate(calculating_charbuf, 63);
            outgoing_frame->crc[0] = (char)(crc);

            char * outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            ll_append_node(outgoing_frames_head_ptr,
                               outgoing_charbuf);
            free(outgoing_frame);
            free(calculating_charbuf);

        }
        free(inframe);
        free(ll_inmsg_node);
    }

}

void * run_receiver(void * input_receiver)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Receiver * receiver = (Receiver *) input_receiver;
    LLnode * outgoing_frames_head;


    //This incomplete receiver thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up if there is nothing in the incoming queue(s)
    //2. Grab the mutex protecting the input_msg queue
    //3. Dequeues messages from the input_msg queue and prints them
    //4. Releases the lock
    //5. Sends out any outgoing messages


    while(1)
    {    
        //NOTE: Add outgoing messages to the outgoing_frames_head pointer
        outgoing_frames_head = NULL;
        gettimeofday(&curr_timeval, 
                     NULL);

        //Either timeout or get woken up because you've received a datagram
        //NOTE: You don't really need to do anything here, but it might be useful for debugging purposes to have the receivers periodically wakeup and print info
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;
        time_spec.tv_sec += WAIT_SEC_TIME;
        time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&receiver->buffer_mutex);

        //Check whether anything arrived
        int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        if (incoming_msgs_length == 0)
        {
            //Nothing has arrived, do a timed wait on the condition variable (which releases the mutex). Again, you don't really need to do the timed wait.
            //A signal on the condition variable will wake up the thread and reacquire the lock
            pthread_cond_timedwait(&receiver->buffer_cv, 
                                   &receiver->buffer_mutex,
                                   &time_spec);
        }

        handle_incoming_msgs(receiver,
                             &outgoing_frames_head);

        pthread_mutex_unlock(&receiver->buffer_mutex);
        
        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames user has appended to the outgoing_frames list
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *) ll_outframe_node->value;
            
            //The following function frees the memory for the char_buf object
            send_msg_to_senders(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);

}
