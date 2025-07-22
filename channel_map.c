#include "channel_map.h"



struct ChannelMap* channel_map_init(int size)
{
    struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
    map->size = size;
    map->list = (struct Channel**)malloc(size * sizeof(struct Channel*));
    return map;
}

void channel_map_clear(struct ChannelMap* map)
{
    if (map!=NULL)
    {
        for (int i=0; i<map->size; i++)
        {
            free(map->list[i]);
        }
        free(map->list);
        map->list = NULL;
    }
    map->size = 0;
}

bool make_map_room(struct ChannelMap* map, int newSize, int unitSize)
{
    if (map->size < newSize)
    {
        int curSize = map->size;
        while (curSize < newSize)
        {
            curSize = curSize * 2;
        }
        struct Channel** tmp = (struct Channel**)realloc(map->list, curSize*unitSize);
        if (tmp == NULL) 
        {
            return false;
        }
        map->list = tmp;
        memset(&map->list[map->size], 0, (curSize-map->size)*unitSize);
        map->size = curSize;
    }
    return true;
}