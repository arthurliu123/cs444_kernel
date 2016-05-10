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

static void sstf_add_request(struct request_queue *q, struct request *rq)
{
   struct sstf_data *sd = q->elevator->elevator_data;
         
        list_add_tail(&rq->queuelist, &sd->queue);
        printk(KERN_INFO "[SSTF] add %s %lu\n",rq->cmd,(long)rq->__sector);
}

static struct request *
noop_former_request(struct request_queue *q, struct request *rq)
{
   struct sstf_data *sd = q->elevator->elevator_data;

   if (rq->queuelist.prev == &sd->queue)
       return NULL;
   return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
noop_latter_request(struct request_queue *q, struct request *rq)
{
   struct sstf_data *sd = q->elevator->elevator_data;

   if (rq->queuelist.next == &sd->queue)
       return NULL;
   return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int noop_init_queue(struct request_queue *q, struct elevator_type *e)
{
   struct sstf_data *sd;
   struct elevator_queue *eq;

   eq = elevator_alloc(q, e);
   if (!eq)
       return -ENOMEM;

   sd = kmalloc_node(sizeof(*sd), GFP_KERNEL, q->node);
   if (!sd) {
       kobject_put(&eq->kobj);
       return -ENOMEM;
   }
   eq->elevator_data = sd;

   INIT_LIST_HEAD(&sd->queue);
        sd->cur_pos = 0;

   spin_lock_irq(q->queue_lock);
   q->elevator = eq;
   spin_unlock_irq(q->queue_lock);
   return 0;
}

static void noop_exit_queue(struct elevator_queue *e)
{
   struct sstf_data *sd = e->elevator_data;

   BUG_ON(!list_empty(&sd->queue));
   kfree(sd);
}

static struct elevator_type elevator_sstf = {
   .ops = {
       .elevator_merge_req_fn      = noop_merged_requests,
       .elevator_dispatch_fn       = sstf_dispatch,
       .elevator_add_req_fn        = sstf_add_request,
       .elevator_former_req_fn     = noop_former_request,
       .elevator_latter_req_fn     = noop_latter_request,
       .elevator_init_fn       = noop_init_queue,
       .elevator_exit_fn       = noop_exit_queue,
   },
   .elevator_name = "sstf",
   .elevator_owner = THIS_MODULE,
};

static int __init sstf_init(void)
{
   return elv_register(&elevator_sstf);
}

static void __exit sstf_exit(void)
{
   elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);


MODULE_AUTHOR("Jiawei Liu");
MODULE_LICENSE("OSU");
MODULE_DESCRIPTION("SSTF IO scheduler");
