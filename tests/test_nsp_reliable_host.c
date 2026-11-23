#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_nsp_reliable.h"
int main(void) {
    struct port_nsp_reliable x;uint16_t seq=99;
    port_nsp_reliable_init(&x);
    assert(x.next_rx==1&&x.next_tx==1);
    assert(port_nsp_rx_sequence(&x,0)==PORT_NSP_RX_OUT_OF_ORDER);
    assert(port_nsp_rx_sequence(&x,1)==PORT_NSP_RX_NEW);
    assert(port_nsp_rx_sequence(&x,1)==PORT_NSP_RX_DUPLICATE);
    assert(port_nsp_rx_sequence(&x,3)==PORT_NSP_RX_OUT_OF_ORDER);
    assert(port_nsp_rx_sequence(&x,2)==PORT_NSP_RX_NEW);
    assert(x.accepted==2&&x.duplicates==1&&x.out_of_order==2);
    assert(port_nsp_rx_sequence(&x,4096)==PORT_NSP_RX_INVALID);
    x.next_rx=4095;
    assert(port_nsp_rx_sequence(&x,4095)==PORT_NSP_RX_NEW);
    assert(x.next_rx==0);
    assert(port_nsp_rx_sequence(&x,0)==PORT_NSP_RX_NEW);
    assert(x.next_rx==1);
    assert(port_nsp_tx_begin(&x,0xfffffff0u,&seq)==0&&seq==1);
    assert(port_nsp_tx_begin(&x,0,&seq)<0);
    assert(port_nsp_tx_ack(&x,2)==0&&x.pending);
    assert(port_nsp_tx_tick(&x,0x00000008u)==0);
    assert(port_nsp_tx_tick(&x,0x000000b8u)==1&&x.attempts==2);
    assert(port_nsp_tx_ack(&x,1)==1&&!x.pending);
    assert(port_nsp_tx_ack(&x,1)==0);
    assert(port_nsp_tx_begin(&x,123,&seq)==0&&seq==2);
    assert(port_nsp_tx_tick(&x,323)==1);
    assert(port_nsp_tx_tick(&x,523)==1);
    assert(port_nsp_tx_tick(&x,723)==-1&&!x.pending);
    assert(x.retries==3&&x.timeouts==1&&x.acked==1);
    assert(port_nsp_tx_begin(&x,800,&seq)==0&&seq==3);
    port_nsp_tx_cancel(&x);
    assert(!x.pending&&x.attempts==0);
    x.next_tx=4095;
    assert(port_nsp_tx_begin(&x,9,&seq)==0&&seq==4095);
    assert(port_nsp_tx_ack(&x,4095)==1);
    assert(port_nsp_tx_begin(&x,10,&seq)==0&&seq==0);
    assert(port_nsp_tx_ack(&x,0)==1);
    puts("M20 NSP reliable stop-and-wait sequence/ACK/retry PASS");
    return 0;
}
