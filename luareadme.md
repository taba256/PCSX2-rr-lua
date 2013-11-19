LuaScriptingFunctions
---
全体的にエラーチェックが微妙であるため注意
##emu/pcsx2
* pcsx2.frameadvance()
 - フレームアドバンスをします。
 - pcsx2.registerbefore、pcsx2.registerafter、pcsx2.registerexitで登録された関数の中では使えません。
* pcsx2.print(string msg)
 - コンソールに文字列を出力します。
* pcsx2.clear()
 - コンソールをクリアします。
* int pcsx2.getframecount()
 - 現在のフレームカウントを取得します。
* pcsx2.pause()
 - ゲームを一時停止します。
* pcsx2.registerbefore(function func)
* pcsx2.registerafter(function func)
* pcsx2.registerexit(function func)
 - CPU実行前、CPU実行後、スクリプト終了時に実行する関数を登録します。
 - 引数をnilにすると登録を取り消せます。

##memory
アドレスは0x00000000-0x01ffffffの間だけ

* int memory.readbyte(int addr) / readbyteunsigned
 - 符号なし1バイト整数を取得します。
* int memory.readbytesigned(int addr)
 - 符号付き1バイト整数を取得します。
* int memory.readword(int addr) / readwordunsigned / readshort / readshortunsigned
 - 符号なし2バイト整数を取得します。
* int memory.readwordsigned(int addr) / readshortsigned
 - 符号付き2バイト整数を取得します。
* int memory.readdword(int addr) / readdwordunsigned / readlong / readlongunsigned
 - 符号なし4バイト整数を取得します。
* int memory.readdwordsigned(int addr) / readlongsigned
 - 符号付き4バイト整数を取得します。
* number memory.readfloat(int addr)
 - 4バイト浮動小数点数を取得します。
* memory.writebyte(int addr, int val)
 - 1バイト整数を書き込みます。
* memory.writeword(int addr, int val) / writeshort
 - 2バイト整数を書き込みます。
* memory.writedword(int addr, int val) / writelong
 - 4バイト整数を書き込みます。
* memory.writefloat(int addr, number val)
 - 4バイト浮動小数点数を書き込みます。

##joypad 
true,false up,right,down,left,triangle,circle,cross,square,l1,l2,l3,r1,r2,r3,start,select  
0-255 lx,ly,rx,ry  
portはコントローラ番号、現状1,2のみ

* table joypad.get(int port)
 - ボタン入力を取得します。
* joypad.set(int port, table buttons)
 - ボタン入力を行います。