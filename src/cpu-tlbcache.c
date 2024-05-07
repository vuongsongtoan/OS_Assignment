/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */


#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))
#define TLB_SIZE mp->maxsz/TLB_ENTRY_SIZE

pthread_mutex_t cache_lock;
// pthread_mutex_t TLBMEMPHY_lock;

// typedef struct {
//    Byte Used;
//    int pid;
//    int pgnum;
//    int frame_number;
// } TLBEntry;


// need 1 bit for detect the entry is used or not: 0 = Not used, 1 = Used
// 32 bit (unsigned int) = 4 bytes for pid
// need 14 bit for pgnum -> 2 bytes
// frame number has 13 bit = 2 byte
// -/----/--/--  => 1 entry hết 9 byte
// pid/pgnum/frame number
/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value    ** Changed to int because frame number has 12 bit
 */
int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, int* value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   pthread_mutex_lock(&cache_lock);
   // Duyệt qua từng tlb_entry trong mp->storage
   for (int i = 0; i < mp->maxsz; i += TLB_ENTRY_SIZE)
   {
      // Trích xuất pid, pgnum, và data từ tlb_entry hiện tại
      int used = mp->storage[i];
      if ((used&1) == 0) continue;
      int entry_pid = 0;
      for (int j = 3; j >= 0; j--){
         entry_pid |= (mp->storage[i + 4 - j] << j*8);
      }
      int entry_pgnum = (mp->storage[i + 5] << 8) | mp->storage[i + 6];
      int entry_data = (mp->storage[i + 7] << 8) | mp->storage[i + 8];

      // Nếu pid và pgnum của tlb_entry hiện tại khớp với pid và pgnum đầu vào
      if (entry_pid == pid && entry_pgnum == pgnum)
      {
         // Đặt value thành data của tlb_entry hiện tại và trả về 0
         *value = entry_data;
         pthread_mutex_unlock(&cache_lock);
         return 0;
      }
   }
   pthread_mutex_unlock(&cache_lock);

   // Nếu không tìm thấy tlb_entry phù hợp, trả về -1
   return -1;
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value   ** Changed to int because frame number has 12 bit
 */
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, int* value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   pthread_mutex_lock(&cache_lock);
   // printf("Cache write:\tPID: %d\tpgnum: %d\tfrn: %d\n", pid, pgnum, *value);
   // Duyệt qua từng tlb_entry trong mp->storage
   // for (int i = 0; i < mp->maxsz; i += TLB_ENTRY_SIZE)
   // {
   //    // Trích xuất pid, pgnum, và data từ tlb_entry hiện tại
   //    int used = mp->storage[i];
   //    // printf("Used: %d, i = %d\n", used, i);
   //    if ((used&1) == 0) continue;
   //    int entry_pid = 0;
   //    for (int j = 0; j <= 3; j++){
   //       entry_pid |= (mp->storage[i + 4 - j] << j*8);
   //    }
   //    int entry_pgnum = (mp->storage[i + 5] << 8) | mp->storage[i + 6];
   //    // printf("Cache entry %d:\tPID: %d\tpgnum: %d\n", used, entry_pid, entry_pgnum);
   //    // Nếu pid và pgnum của tlb_entry hiện tại khớp với pid và pgnum đầu vào
   //    if (entry_pid == pid && entry_pgnum == pgnum)
   //    {
   //       // Đặt value thành data của tlb_entry hiện tại và trả về 0
   //       // printf((((*value)) & 0xFF));
   //       TLBMEMPHY_write(mp, i + 7, (((*value) >> 8) & 0xFF));  // Lấy 4 bit đầu
   //       TLBMEMPHY_write(mp, i + 8, ((*value) & 0xFF));  // Lấy byte cuối
   //       // mp->storage[i + 8] = ((*value) & 0xFF); 
   //       // printf("HIT\n");
   //       pthread_mutex_unlock(&cache_lock);
   //       return 0;
   //    }
   // }
   // printf("Miss\n");
   // Miss
   // Thêm vào TLB cache
   int free_entry = -1;
   for (int i = 0; i < mp->maxsz; i += TLB_ENTRY_SIZE){
      if ((mp->storage[i]&1) == 0){
         free_entry = i/TLB_ENTRY_SIZE;
         break;
      }
   }
   // printf("free_entry: %d\n", free_entry);
   if (free_entry == -1){ // TLB đầy
      free_entry = rand()%TLB_SIZE;
   }
   int base_entry_addr = free_entry*TLB_ENTRY_SIZE;
   // Viết pid vào cache
   for (int i = 0; i <= 3; i++){
      // printf("index: %d, val: %d\n", 4 - i, (pid >> i*8) & 0xFF);
      TLBMEMPHY_write(mp, base_entry_addr + 4 - i, (pid >> i*8) & 0xFF); 
   }
   // Viết pgnum vào cache
   TLBMEMPHY_write(mp, base_entry_addr + 6, pgnum & 0xFF);
   TLBMEMPHY_write(mp, base_entry_addr + 5, (pgnum >> 8) & 0xFF);
   // Viết value (aka frame number) vào cache
   TLBMEMPHY_write(mp, base_entry_addr + 8, *value & 0xFF);
   TLBMEMPHY_write(mp, base_entry_addr + 7, (*value >> 8) & 0xFF);

   // Đánh dấu entry đã được sử dụng
   TLBMEMPHY_write(mp, base_entry_addr, 1);
   pthread_mutex_unlock(&cache_lock);
   return 0;
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   // printf("TLBMEMPHY_read\n");
   if (mp == NULL)
      return -1;

   /* TLB cached is random access by native */
   *value = mp->storage[addr];
   return 0;
}


/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
      return -1;

   /* TLB cached is random access by native */
   mp->storage[addr] = data;
   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */


int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */
   // Kiểm tra xem bộ nhớ vật lý có tồn tại hay không
   if(mp == NULL || mp->storage == NULL)
   {
      printf("Physical memory doesn't exist.\n");
      return -1;
   }
   printf("TLBMEMPHY_dump\n");
   pthread_mutex_lock(&cache_lock);
   // Duyệt qua từng tlb_entry trong mp->storage
   for (int i = 0; i < mp->maxsz; i += TLB_ENTRY_SIZE)
   {
      // Trích xuất pid, pgnum, và data từ tlb_entry hiện tại
      int used = mp->storage[i];
      if ((used&1) == 0) continue;
      int entry_pid = 0;
      for (int j = 0; j <= 3; j++){
         entry_pid |= (mp->storage[i + 4 - j] << j*8);
      }
      int entry_pgnum = (mp->storage[i + 5] << 8) | mp->storage[i + 6];
      int entry_data = (mp->storage[i + 7] << 8) | mp->storage[i + 8];

      printf("Entry %d:\tUsed: %d\tEntry pid: %d\tEntry pagenum: %d\tEntry framenum: %d\n", i/TLB_ENTRY_SIZE, used, entry_pid, entry_pgnum, entry_data);
   }
   pthread_mutex_unlock(&cache_lock);
   return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   mp->rdmflg = 1;
   pthread_mutex_init(&cache_lock, NULL);
   return 0;
}

//#endif
