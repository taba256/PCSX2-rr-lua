#PCSX2-rr-lua

---
PCSX2-rr-luaは、[PCSX2-rr][1]をベースにTAS機能の安定化やLuaスクリプト機能の追加など、なんかそれっぽく改造したものです。  
(ムービーファイルやステートセーブの互換性は)ないです

###現状:
* 2P記録を可能にした
 - 再生時、一部ゲームで2P側が全ボタン押しになる問題が解消。
* 起動時刻を記録するようにした
 - 乱数のシードに時刻を使っているゲームにおいて、乱数の違いによるDesyncはなくなるはず。
* Luaスクリプト機能
 - フレームアドバンスなどの基本機能
 - メモリ読み書き
 - パッド入出力

###ToDo:
* いろいろの安定化
* ソースの可読性向上

[1]: http://code.google.com/p/pcsx2-rr/ "PCSX2-rr"