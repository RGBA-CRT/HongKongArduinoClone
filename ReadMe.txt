/*-------------------------------------------------------------------------------------
					HongKongArduino ������ [2016/7/20]
	
	�I���W�i���̍�� : (c) 2014 ���ɂ��  [http://hongkongarduino.web.fc2.com/]
	�@�����ō�Ҏҁ@ : RGBA_CRT 2016/7/20 [rgba3crt1p@gmail.com]
-------------------------------------------------------------------------------------*/

���T�v
	���̃v���O�����͂��ɂ�܎����J���E���J���ꂽ�A"���[�ǂˁ[�ށu�z���R���v with Arduino"�Ɏ��������
	�������ESRAM�̋z�o���Ȃǂ̋@�\�����������̂ł��B

	���[�ǂˁ[�ށu�z���R���v with Arduino
	http://hongkongarduino.web.fc2.com/


������
	�{�v���O�������g�p���Ă����Ȃ�j���E�ЊQ�����������Ƃ��Ă�RGBA_CRT�͈�ؐӔC�����܂���B
	���ɃJ�[�g���b�W�����̃Z�[�u�f�[�^�̈�̃f�[�^�͏����₷���̂ŁA���̂��Ƃ𓥂܂�����ł��g�p���������B
	���ׂẴJ�[�g���b�W�ɂ����Đ��m��ROM�ESRAM�̋z�o����ۏ؂͂��܂���B
	����SRAM�̏������݋@�\�͕s����Ȃ̂ŏ����Ăق����Ȃ��f�[�^���J�[�g���b�W�ɂ���ꍇ�ɂ͎g�p���Ȃ��ł��������B
	�܂��A�z���o����ROM�C���[�W���C���^�[�l�b�g��Ɍ��J����ƒ��쌠�@�ᔽ�ɂȂ�̂ŃA�b�v���[�h�͂��Ȃ��ł��������B


���Z�b�g���e
	HongKongArduinoClone.exe		->	Windows�p�z�o���v���O�����ł��B
	HongKongArduinoCloneSrc.zip	->	��L�t�@�C���̃\�[�X�R�[�h�ł��B
	
	HongKongArduinoFast\ArduinoHexWriter.exe	->	Arduino�փt�@�[���E�F�A���������ނ��߂̃o�b�`�t�@�C���쐬�v���O�����ł��B
	HongKongArduinoFast\ArduinoHexWriterSrc.zip	->	��L�t�@�C���̃\�[�X�R�[�h�ł��B
	HongKongArduinoFast\HongKongArduinoFast.hex	->	�R���p�C���ς�Arduino�p�t�@�[���E�F�A�ł��B�����Arduino�֏�������ł��������B
	HongKongArduinoFast\HongKongArduinoFast.ino	->	��L�t�@�C���̃\�[�X�R�[�h�ł��B


������m�F�ς݃J�[�g���b�W
	�E�m�[�}����LoROM�J�[�g���b�W
	�EMAD1����HiROM�J�[�g���b�W
	�EDSP1���ڃJ�[�g���b�W
	�ESuperFX���ڃJ�[�g���b�W
	��L�ȊO�̃J�[�g���b�W�͓���e�X�g�����Ă��܂���B
	SA-1���ڃJ�[�g���b�W�i�J�[�r�B3�A�}���IRPG�Ȃǁj�͉�H�I�ȗ��R�ŋz�o���s�\�ł��B


���g����
	��ɉ�H���������A�����̃N���C�A���g�iWinHongKong�j�ł̐���ȋz�o�����ł��邱�Ƃ��O��ł��B

	1.Arduino�Ƀt�@�[���E�F�A����������
		HongkongArduinoFast�t�H���_��	HongKongArduinoFast.hex�Ƃ����t�@�C��������̂ŁAArduino��ڑ����ď�������ł��������B
		������̂ݍs���Ă��������B
		
		ArduinoHexWriter.exe���g���Ɣ�r�I�ȒP�ɏ������߂܂��B
			ArduinoIDE�t�H���_�́Aarduino.exe������t�H���_���w�肵�Ă��������B�m�F�{�^���������āu����v�ƕ\������邱�Ƃ��m�F���Ă��������B
			ArduinoHexWriter���N�����A��ԉ��̃e�L�X�g�{�b�N�X��hex�t�@�C�����h���b�v����ƃp�X�����͂���܂��B
			�u�������݁v�{�^����������Arduino��HEX�t�@�C�����������݂܂��B
			�R�}���h�v�����v�g���\������A�uavrdude.exe done. Thank you.�v�ƕ\�����ꂽ�珑�����݂ɐ������Ă��܂��B
			�����L�[�������ďI�����Ă��������B
		
		�Ȃ��A����HEX�t�@�C���͍œK���I�v�V����-O3(�������x�D��)�ŃR���p�C������Ă��܂��B
		�f�t�H���g��ArduinoIDE�̐ݒ�ł͍œK���I�v�V������-Os(�T�C�Y�̏������D��)�ɂȂ��Ă��܂��B
		
	2.�N���C�A���g�̋N��
		HongKongArduinoClone.exe���N�����Ă��������B
		�uSerial Port�v��Arduino�̃|�[�g���w�肵��[connect]�{�^���������Ă��������B
		�E��Infomation���ɁuConnection successful!�v���\������A�Q�[���^�C�g�����\������Ă���ΐڑ������ł��B
		�����A�t�@�[���E�F�A��ύX���ĒʐM���[�g��ύX�����ꍇ�̓_�C�A���O�E��[baudrate]��ύX���Ă��������B
		
	3.�N���C�A���g�̑���
		3-1 �J�[�g���b�W��ڑ����AGetCartInfo�{�^���������ăQ�[�������擾���Ă��������B��������Ȃ���LoROM/HiROM�̔��肪����܂���B
		3-2 DumpROM�ŋz�o�����s���܂��B
		3-3 DumpSRAM��SRAM�̋z�o�����s���܂��B���܂��z���o���Ȃ������ꍇ��MAD-1�̃`�F�b�N�����ċz�o�������Ă݂Ă��������B
		
	4.�I��
		[Close]�{�^����������Arduino�Ƃ̐ڑ���ؒf���܂��B
		

���J����
	����\���̖ڈ��ɂ��Ă��������B
	OS 	: Win7 SP1�@32bit
	CPU 	: Core i3 330M
	Arduino : Aitendo����Arduino UNO�݊��@�iATMega328P 16MHz�j
	ArduinoIDE : ver1.6.6
	HongKongArduino�Ƃ̒ʐM���x�F54179byte/sec
	�J������FActiveBasic ver4 + �����p���C�u����

��Q&A
	�E�z�o������[0]Warning!�@�Əo�邪�A����͉����H
		���M����Ă����f�[�^�̃T�C�Y���s���̎��ɕ\������܂��B
		
	�E�uOver retry count.�v�ƕ\������ċz�o������~����
		��L��Warning��20��قǔ��������ꍇ�ɋz�o������~���܂��B
		HongKongArduinoClone.ini�̒���MaxContinue�̉E�ӂ̒l�𑝂₷���Ƃ�Warning���̍Ď��s�񐔂𑝂₹�܂��B
		
	�ESerial Port�̗���Err�ƕ\�������
		COM�|�[�g�̃��X�g�A�b�v�ɕK�v�ȃ��������s�����Ă���\��������܂��B
		HongKongArduinoClone.ini�̒���QDDSize�̉E�ӂ�����������Ɖ��P����\��������܂��B
		�Ȃ��AQDDSize�̒l��1023�ȉ��̏ꍇ�f�t�H���g�̒l�ɖ߂���܂��̂Œ��ӂ��Ă��������B
		
	�E�Q�[���^�C�g���Ȃǂ�����ɕ\������Ȃ�
	�E�z���o�����f�[�^��CRC������Ȃ�
		�J�Z�b�g�̐ڐG�������\��������܂��B�Ȗ_�ɃT�r���܂Ȃǂ����Ē[�q��|�����Ă��������B
		�܂��A��������ڑ���؂�AUSB�P�[�u���𔲂����̂��A������x�Đڑ�����Ɖ��P����ꍇ������܂��B
		����ł��_���ȏꍇ�͂�����߂Ă��������B
		
	�E�Q�[��������ɋz���o���Ȃ�
		��Ή��̃J�[�g���b�W�̏ꍇ������܂��B���Ƃ��΁ASA-1���ڃJ�[�g���b�W�Ȃǂ͐��䂪������ߑΉ����Ă��܂���B
		
	�E�Z�[�u�f�[�^�iSRAM�j������ɋz���o���Ȃ�
		SRAM�̋z�o���̓���m�F�ς݂̃J�[�g���b�W�́A
		�E�m�[�}��LoROM�J�[�g���b�W
		�EMAD1����HiROM�J�[�g���b�W
		�ł��B
		�܂��AMAD1�𓋍ڂ��Ă��Ȃ��̂�MAD1�Ƀ`�F�b�N�����Ȃ���SRAM���z���o���Ȃ��J�[�g���b�W������܂����B
		
	�E�o�O������/�X�y�����Ԉ���Ă���/�v�]������
		��҂ł���RGBA_CRT�֘A��[rgba3crt1p@gmail.com]���Ă��������B�������A������Ƃ͌���܂���B
		�Ȃ��A�����łɊւ���A���̓I���W�i���̍�҂ł��� ���ɂ�܎��ɂ͖��f�ɂȂ�̂ł��Ȃ��ł��������B
		
	�E�\�[�X�R�[�h�̃��C�Z���X�́H
		Arduino�p�̃t�@�[���E�F�A�͂��ɂ�܎��̐��앨�ł���̂ŁA���łƓ������u�N���G�C�e�B�u�E�R�����Y �\�� - ��c�� 4.0 ���� ���C�Z���X�v���K�p����܂��B
		HongKongArduinoClone�͂��ɂ�܎���WinHongKongArduino�̃\�[�X�R�[�h�⎑�����Q�l�ɍ�������̂Ȃ̂œ�����
		�u�N���G�C�e�B�u�E�R�����Y �\�� - ��c�� 4.0 ���� ���C�Z���X�v��K�p���܂��B
		
	�E�ʐM���x��ύX������
		HongKongArduinoFast.ino �� setup()����Serial.begin(500000);���ʐM���x�ł��B
		���D�݂ŕύX���Ă��������B
		�Ȃ��AArduinoIDE�̃f�t�H���g�̍œK���I�v�V������-Os�Ȃ̂�-O3�ɐݒ�ύX���邱�Ƃ������߂��܂��B
		
	�EHongKongArduinoClone.exe�̃\�[�X�R�[�h�ɂ���
		ActiveBasic ver4�ŊJ�����Ă��܂��B[http://www.activebasic.com/ab4_download.html]
		���@��BASIC�̔�����Ԃ���C����Ƃ����悤�Ȋ����Ȃ̂ł����킩��Ǝv���܂��B
		
		
	
���ӎ�
	���̃v���O���������ɂ������āA���ɂ�܎���WinHongKongArduino�̃\�[�X�R�[�h�A
	�����āu��H�̓��쌴���ƃv���O�����쐬�̃q���g�v��p�������ł̃z���R���̃\�[�X�R�[�h���Q�l�ɂ��č��܂����B
	��H�̕��������ɕ׋��ɂȂ�܂����B�����Ɋ��ӂ̈ӂ�\���܂��B
	
	
���A����
	Web : http://rgbacrt.seesaa.net/
	E-mail : rgba3crt1p@gmail.com
	

�� ����
	[2016/3/23]ver 0.0 - ���񃊃��[�X
	[2016/7/20]ver 0.1 - �`�F�b�N�T������@�\�ǉ�