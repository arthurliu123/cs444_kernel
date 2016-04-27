#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct sstf_data {
        struct list_head queue;
        sector_t cur_pos;
};

static void noop_merged_requests(struct request_queue *q, struct request *rq,
                struct request *next)
{
   list_del_init(&next->queuelist);
}

static int sstf_dispatch(struct request_queue *q, int force)
{
   struct sstf_data *sd = q->elevator->elevator_data;
        struct list_head *lh = &sd->queue;
        sector_t cur_pos = sd->cur_pos;

   if (!list_empty(lh)) {
       struct request *rq;
                struct request *ready_rq = NULL;
       struct list_head *p;
                sector_t rq_pos;
                sector_t pos_diff = -1;
                sector_t temp;
                
                printk(KERN_INFO "[SSTF] Current sector is: %ld", (long) cur_pos);
                list_for_each(p, lh){
                        rq = list_entry(p,struct request, queuelist);
                        rq_pos = blk_rq_pos(rq);
                        temp = rq_pos > cur_pos ? (rq_pos - cur_pos) : (cur_pos - rq_pos);
                        if(pos_diff > temp){
                                pos_diff = temp;
                                ready_rq = rq;
                        }
                        printk(KERN_INFO "[SSTF] Request sector: %ld, diff = %ld", (long) rq->__sector,(long) temp);
                }

                if (ready_rq == NULL){
                        printk (KERN_WARNING "Unexpected error!\n");
                        return 0;
                }else{
                        printk(KERN_INFO "[SSTF] dispatch %s %ld\n",ready_rq->cmd,(long)ready_rq->__sector);
                        sd->cur_pos = blk_rq_pos(ready_rq);
                        list_del_init(&ready_rq->queuelist);
                        elv_dispatch_sort(q, ready_rq);
                        return 1;
                }
                printk(KERN_INFO "\n");
                
   }
   return 0;
}


module_init(sstf_init);
module_exit(sstf_exit);


MODULE_AUTHOR("Jiawei Liu");
MODULE_LICENSE("OSU");
MODULE_DESCRIPTION("SSTF IO scheduler");
