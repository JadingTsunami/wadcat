/*
wadcat: Command-line WAD lump printer
Copyright (C) 2021 Jading Tsunami

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

FILE* f;
bool short_print = false;
char line_sep = '\n';
char* target_lump = NULL;
char* target_map = NULL;
typedef enum {
    TARGET_LUMPS,
    TARGET_MAPS_ONLY,
    TARGET_THINGS,
    TARGET_LINEDEFS,
    TARGET_SIDEDEFS,
    TARGET_VERTEXES,
    TARGET_SECTORS,
    TARGET_LAST
} wadtarget_t;

void parseHeader(bool* iwad, int* numlumps, int* diroff)
{
    char hdr[5];
    fread(hdr,4,1,f);
    hdr[4]='\0';
    if(!short_print)
        printf("Header: %s\n", hdr);
    if( strcmp(hdr,"IWAD")==0 ) *iwad=true;
    else *iwad=false;

    fread(numlumps,1,4,f);
    fread(diroff,1,4,f);
}

char* str_upper(char* s)
{
    char* begin = s;
    while (s && *s) {
        *s = toupper(*s);
        s++;
    }
    return begin;
}

bool checkPattern(char* lump, char* pattern)
{
    int watchdog = 0;
    while(*lump) {
        if(((*pattern == 'x') && (*lump >= '0' || *lump <= '9'))
                ||
           (*pattern == *lump)) {
            lump++;
            pattern++;
        } else {
            return false;
        }
        watchdog++;
        /* lumps max at 8 chars */
        if(watchdog == 8)
            break;
    }
    return true;
}

bool isMAPxx(char* lump)
{
    return checkPattern(lump, "MAPxx");
}

bool isExMx(char* lump)
{
    return checkPattern(lump, "ExMx");
}

bool isMapLump(char* lump)
{
    char* maplumps[] = {
        "THINGS",
        "LINEDEFS",
        "SIDEDEFS",
        "VERTEXES",
        "SEGS",
        "SSECTORS",
        "NODES",
        "SECTORS",
        "REJECT",
        "BLOCKMAP",
        "BEHAVIOR",
        "SCRIPTS",
        "LEAFS",
        "LIGHTS",
        "MACROS",
        "GL_VERT",
        "GL_SEGS",
        "GL_SSECT",
        "GL_NODES",
        "GL_PVS",
        NULL
    };
    for(int i = 0; maplumps[i]; i++) {
        if(checkPattern(lump, maplumps[i]))
            return true;
    }
    return false;
}

void getLump(int pos, int bsize, uint8_t* contents)
{
    assert(f);
    assert(!feof(f));
    int oldpos = ftell(f);
    fseek(f, pos, SEEK_SET);
    assert(fread(contents, sizeof(uint8_t), bsize, f) == bsize);
    fseek(f, oldpos, SEEK_SET);
}

void printSectors(int pos, int bsize, bool print_header)
{
    int16_t floor_height;
    int16_t ceiling_height;
    int8_t floor_texture[9];
    int8_t ceiling_texture[9];
    int16_t lightlevel;
    int16_t special;
    int16_t tag;


    if (print_header) {
        printf("%-6s | %-6s | %-6s | %-8s | %-8s | %-6s | %-6s | %-6s%c", "Sector", "Floor", "Ceil", "FTex", "CTex", "Light", "Spc", "Tag", line_sep);
    }

    uint8_t* sectors = (uint8_t*) malloc(sizeof(uint8_t)*bsize);
    uint32_t num_sectors = bsize/26;
    uint32_t offset = 0;
    assert(sectors);
    getLump(pos, bsize, sectors);

    for(int i = 0; i < num_sectors; i++) {
        offset = i*26;
        floor_height = sectors[offset + 0] | (sectors[offset + 1] << 8);
        ceiling_height =  sectors[offset + 2] | (sectors[offset + 3] << 8);
        memcpy( floor_texture, &sectors[offset + 4], 8);
        memcpy( ceiling_texture, &sectors[offset + 12], 8);
        floor_texture[8] = '\0';
        ceiling_texture[8] = '\0';
        lightlevel = sectors[offset + 20] | (sectors[offset + 21] << 8);
        special = sectors[offset + 22] | (sectors[offset + 23] << 8);
        tag = sectors[offset + 24] | (sectors[offset + 25] << 8);
        printf("% 6d | % 6d | % 6d | %-8s | %-8s | % 6d | % 6d | % 6d%c", i, floor_height, ceiling_height, floor_texture, ceiling_texture, lightlevel, special, tag, line_sep);
    }
    free(sectors);
}

void printThings(int pos, int bsize, bool print_header)
{
    if (print_header) {
        printf("%-6s | %-6s | %-6s | %-6s | %-6s | %-6s%c", "Thing", "X pos", "Y pos", "Angle", "Type", "Flags", line_sep);
    }

    uint16_t* things = (uint16_t*) malloc(sizeof(uint16_t)*bsize/2);
    uint32_t num_things = bsize/10;
    uint32_t offset = 0;
    assert(things);
    getLump(pos, bsize, (uint8_t*) things);

    for(int i = 0; i < num_things; i++) {
        offset = i*5;
        printf("% 6d | % 6d | % 6d | % 6d | % 6d | %06x%c", i, things[offset + 0], things[offset + 1], things[offset + 2], things[offset + 3], things[offset + 4], line_sep);
    }
    free(things);
}

void printLinedefs(int pos, int bsize, bool print_header)
{
    if (print_header) {
        printf("%-7s | %-6s | %-6s | %-4s | %-6s | %-6s | %-6s | %-6s%c", "Linedef", "Start", "End", "Flag", "Spc", "Tag", "Front", "Back", line_sep);
    }

    int16_t* linedefs = (int16_t*) malloc(sizeof(int16_t)*bsize/2);
    uint32_t num_linedefs = bsize/14;
    uint32_t offset = 0;
    assert(linedefs);
    getLump(pos, bsize, (uint8_t*) linedefs);

    for(int i = 0; i < num_linedefs; i++) {
        offset = i*7;
        printf("%-7d | %-6d | %-6d | %04x | %-6d | %-6d | %-6d | %-6d%c",
                i,
                linedefs[offset + 0],
                linedefs[offset + 1],
                linedefs[offset + 2],
                linedefs[offset + 3],
                linedefs[offset + 4],
                linedefs[offset + 5],
                linedefs[offset + 6],
                line_sep
                );
    }
    free(linedefs);
}

void printSidedefs(int pos, int bsize, bool print_header)
{
    if (print_header) {
        printf("%-7s | %-6s | %-6s | %-8s | %-8s | %-8s | %-6s%c", "Sidedef", "X", "Y", "Upper", "Lower", "Middle", "Sector", line_sep);
    }

    int8_t* sidedefs = (int8_t*) malloc(sizeof(int8_t)*bsize);
    uint32_t num_sidedefs = bsize/30;
    uint32_t offset = 0;
    assert(sidedefs);
    getLump(pos, bsize, (uint8_t*) sidedefs);

    int8_t midtex[9];
    int8_t bottex[9];
    int8_t toptex[9];
    for(int i = 0; i < num_sidedefs; i++) {
        offset = i*30;
        memcpy(toptex, &sidedefs[offset +  4], 8);
        memcpy(bottex, &sidedefs[offset + 12], 8);
        memcpy(midtex, &sidedefs[offset + 20], 8);
        toptex[8] = '\0';
        bottex[8] = '\0';
        midtex[8] = '\0';
        printf("%-7d | %-6d | %-6d | %-8s | %-8s | %-8s | %-6d%c",
                i,
                sidedefs[offset + 0] | (sidedefs[offset + 1] << 8),
                sidedefs[offset + 2] | (sidedefs[offset + 3] << 8),
                toptex,
                bottex,
                midtex,
                sidedefs[offset + 28] | (sidedefs[offset + 29] << 8),
                line_sep
                );
    }
    free(sidedefs);
}

void printVertexes(int pos, int bsize, bool print_header)
{
    if (print_header) {
        printf("%-6s | %-6s | %-6s%c", "Vertex", "X", "Y", line_sep);
    }

    int16_t* vertexes = (int16_t*) malloc(sizeof(int16_t)*bsize/2);
    uint32_t num_vertexes = bsize/4;
    uint32_t offset = 0;
    assert(vertexes);
    getLump(pos, bsize, (uint8_t*) vertexes);

    for(int i = 0; i < num_vertexes; i++) {
        offset = i*2;
        printf("% 6d | % 6d | % 6d%c", i, vertexes[offset + 0], vertexes[offset + 1], line_sep);
    }
    free(vertexes);
}

void printLump(int pos, int bsize, char* lumpname, bool print_header)
{
    if (short_print) {
        printf("%s%c",lumpname,line_sep);
    } else {
        if (print_header) {
            printf("Pos      | Size     | Name%c", line_sep);
            printf("------------------------------%c", line_sep);
        }
        printf("%08X | %08X | %s%c",pos,bsize,lumpname,line_sep);
    }
}

void printLumpRaw(int pos, int bsize, char* lumpname, bool print_header)
{
    uint8_t* lumpdata = (uint8_t*) malloc(sizeof(uint8_t)*bsize);
    assert(lumpdata);
    getLump(pos, bsize, lumpdata);
    int bcnt = 0;
    while (bcnt < bsize) {
        printf("%c", *lumpdata++);
        bcnt++;
    }
}

void processLump(wadtarget_t target, int pos, int bsize, char* lumpname)
{
    static bool first = true;
    switch (target) {
        case TARGET_LUMPS:
            if (!target_lump) {
                printLump(pos, bsize, lumpname, first);
                first = false;
            } else if (target_lump && checkPattern(lumpname, target_lump)) {
                printLumpRaw(pos, bsize, lumpname, first);
                first = false;
            }
            break;
        case TARGET_MAPS_ONLY:
            if(isExMx(lumpname)||isMAPxx(lumpname)) {
                printLump(pos, bsize, lumpname, first);
                first = false;
            }
            break;
        case TARGET_THINGS:
            if(checkPattern(lumpname, "THINGS")) {
                printThings(pos, bsize, first);
                first = false;
            }
            break;
        case TARGET_SECTORS:
            if(checkPattern(lumpname, "SECTORS")) {
                printSectors(pos, bsize, first);
                first = false;
            }
            break;
        case TARGET_LINEDEFS:
            if(checkPattern(lumpname, "LINEDEFS")) {
                printLinedefs(pos, bsize, first);
                first = false;
            }
            break;
        case TARGET_SIDEDEFS:
            if(checkPattern(lumpname, "SIDEDEFS")) {
                printSidedefs(pos, bsize, first);
                first = false;
            }
            break;
        case TARGET_VERTEXES:
            if(checkPattern(lumpname, "VERTEXES")) {
                printVertexes(pos, bsize, first);
                first = false;
            }
            break;
        default:
            break;
    }
}

void processDirectory(int numlumps, wadtarget_t target, char* maptarget)
{
    /* check if we're in the target map or not */
    bool in_target_map = (target_map == NULL);
    for( int i = 0; i < numlumps&&!feof(f); i++ ) {
        int pos, bsize;
        char lumpname[9];
        fread(&pos,1,4,f);
        fread(&bsize,1,4,f);
        fread(lumpname,1,8,f);
        lumpname[8]='\0';

        if (target_map) {
            if (checkPattern(lumpname, target_map)) {
                in_target_map = true;
            } else if ((isExMx(lumpname) || isMAPxx(lumpname)) || !isMapLump(lumpname)) {
                in_target_map = false;
            }
        }

        if (in_target_map) {
            processLump(target, pos, bsize, lumpname);
        }
    }
}

int main(int argc, char** argv)
{
    int c;
    bool help = false;
    wadtarget_t target = TARGET_LUMPS;

    opterr = 0;

    while ((c = getopt (argc, argv, "hltM:mSsqvn:x:")) != -1) {
        switch (c) {
            case 'x':
                short_print = true;
                target = TARGET_LUMPS;
                free(target_lump);
                target_lump = str_upper(strdup(optarg));
                break;
            case 'm':
                target = TARGET_MAPS_ONLY;
                break;
            case 'M':
                free(target_map);
                target_map = strdup(optarg);
                break;
            case 'S':
                target = TARGET_SECTORS;
                break;
            case 't':
                target = TARGET_THINGS;
                break;
            case 'n':
                line_sep = *optarg;
                break;
            case 's':
                target = TARGET_SIDEDEFS;
                break;
            case 'l':
                target = TARGET_LINEDEFS;
                break;
            case 'v':
                target = TARGET_VERTEXES;
                break;
            case 'q':
                short_print = true;
                break;
            case 'h':
                help = true;
                break;
            case '?':
                if (optopt == 'n' || optopt == 'M') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                    break;
                }
                /* fall through */
            default:
                fprintf(stderr, "Unknown argument: %c\n", c);
                break;
        }
    }

    if( argc == optind || help ) {
        printf("Usage: %s [options] [file.wad ...]\n",argv[0]);
        printf("\n");
        printf("Options:\n");
        printf("\t-m: Maps only\n");
        printf("\t\tPrint only map names present in WAD.\n");
        printf("\t-q: Short print\n");
        printf("\t\tPrint only lump names.\n");
        printf("\t-n[c]: Line separator (default: newline)\n");
        printf("\t\tSeparate lump prints by supplied line separator.\n");
        printf("\t\tUseful for computer-readable or batch processing.\n");
        printf("\t-M[MAPNAME]: Select a specific map to print.");
        printf("\t\tPrint only contents from map with header matching MAPNAME.\n");
        printf("\t\tUse 'x' as a substitute for a number (e.g., MAPxx)\n");
        printf("\t-x[LUMPNAME]: Raw print contents of all lumps named LUMPNAME.\n");
        printf("\t\tAlso auto-enables short print mode (-q).\n");
        printf("\t-h: Help\n");
        printf("\t\tPrint this help screen.\n");
        printf("Map decoding options:\n");
        printf("\t-S: Decode and print only SECTORS lump(s)\n");
        printf("\t-t: Decode and print only THINGS lump(s)\n");
        printf("\t-s: Decode and print only SIDEDEFS lump(s)\n");
        printf("\t-l: Decode and print only LINEDEFS lump(s)\n");
        printf("\t-v: Decode and print only VERTEXES lump(s)\n");
        return -1;
    }

    bool multiple_files = ((argc - optind) > 1);
    while(optind < argc) {
        if(multiple_files && !short_print) {
            printf("File: %s\n",argv[optind]);
        }
        f = fopen( argv[optind++], "rb" );
        if(f && !ferror(f)) {
            bool iwad;
            int nl, dir;
            parseHeader(&iwad,&nl,&dir);
            if( !short_print )
                printf( "%s | Lumps: %d | Diroffs: %08x\n", (iwad?"IWAD":"PWAD"),nl,dir);
            fseek(f,dir,0);
            processDirectory(nl, target, NULL);
            fclose(f);
        } else {
            printf("Error opening %s\n",argv[optind-1]);
        }
    }
}
