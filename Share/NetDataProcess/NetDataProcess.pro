# 用于编译 Share 目录到Debug或者Release
TEMPLATE = aux

include(../../../Include/FrameWork.pri)
QMAKE_POST_LINK += $$PATH_PYTHON $$IDE_ROOT/CompileHelper.py $$OUT_DIR_NAME $$_PRO_FILE_PWD_ Share
