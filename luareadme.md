LuaScriptingFunctions
---
�S�̓I�ɃG���[�`�F�b�N�������ł��邽�ߒ���
##emu/pcsx2
* pcsx2.frameadvance()
 - �t���[���A�h�o���X�����܂��B
 - pcsx2.registerbefore�Apcsx2.registerafter�Apcsx2.registerexit�œo�^���ꂽ�֐��̒��ł͎g���܂���B
* pcsx2.print(string msg)
 - �R���\�[���ɕ�������o�͂��܂��B
* pcsx2.clear()
 - �R���\�[�����N���A���܂��B
* int pcsx2.getframecount()
 - ���݂̃t���[���J�E���g���擾���܂��B
* pcsx2.pause()
 - �Q�[�����ꎞ��~���܂��B
* pcsx2.registerbefore(function func)
* pcsx2.registerafter(function func)
* pcsx2.registerexit(function func)
 - CPU���s�O�ACPU���s��A�X�N���v�g�I�����Ɏ��s����֐���o�^���܂��B
 - ������nil�ɂ���Ɠo�^���������܂��B

##memory
�A�h���X��0x00000000-0x01ffffff�̊Ԃ���

* int memory.readbyte(int addr) / readbyteunsigned
 - �����Ȃ�1�o�C�g�������擾���܂��B
* int memory.readbytesigned(int addr)
 - �����t��1�o�C�g�������擾���܂��B
* int memory.readword(int addr) / readwordunsigned / readshort / readshortunsigned
 - �����Ȃ�2�o�C�g�������擾���܂��B
* int memory.readwordsigned(int addr) / readshortsigned
 - �����t��2�o�C�g�������擾���܂��B
* int memory.readdword(int addr) / readdwordunsigned / readlong / readlongunsigned
 - �����Ȃ�4�o�C�g�������擾���܂��B
* int memory.readdwordsigned(int addr) / readlongsigned
 - �����t��4�o�C�g�������擾���܂��B
* number memory.readfloat(int addr)
 - 4�o�C�g���������_�����擾���܂��B
* memory.writebyte(int addr, int val)
 - 1�o�C�g�������������݂܂��B
* memory.writeword(int addr, int val) / writeshort
 - 2�o�C�g�������������݂܂��B
* memory.writedword(int addr, int val) / writelong
 - 4�o�C�g�������������݂܂��B
* memory.writefloat(int addr, number val)
 - 4�o�C�g���������_�����������݂܂��B

##joypad 
true,false up,right,down,left,triangle,circle,cross,square,l1,l2,l3,r1,r2,r3,start,select  
0-255 lx,ly,rx,ry  
port�̓R���g���[���ԍ��A����1,2�̂�

* table joypad.get(int port)
 - �{�^�����͂��擾���܂��B
* joypad.set(int port, table buttons)
 - �{�^�����͂��s���܂��B