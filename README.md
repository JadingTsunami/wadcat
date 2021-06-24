# wadcat: command-line tool to see what's in a WAD file

Print the contents of WAD files, including MAP-specific lumps (SECTORS, THINGS, LINEDEFS, SIDEDEFS, VERTEXES).

## Examples

The situation is this: You download a WAD but don't know what is inside it (which map(s), etc.). wadcat can tell you.

```
$ wadcat -qm test.wad
MAP10
```

Or maybe you are doing a community project and want to replace the flat SLIME16, but don't know if anyone has used it. wadcat + grep can tell you.

```
$ wadcat -S maps.wad  | grep SLIME16
Sector | Floor  | Ceil   | FTex     | CTex     | Light  | Spc    | Tag
    68 |     -4 |    128 | SLIME16  | CEIL3_5  |    160 |      0 |      0
    71 |     -4 |     80 | SLIME16  | DEM1_4   |    160 |      0 |      0
```

Or you want to know how many Chaingunners are in your map.

```
$ wadcat -t map.wad | cut -f5 -d\| | grep 65 | wc -l
      18
```

And so on.

# License

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
