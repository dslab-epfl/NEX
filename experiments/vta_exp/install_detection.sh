export MODEL_NAME=yolov3-tiny
export REPO_URL=https://github.com/dmlc/web-data/blob/main/darknet/
CUR_DIR=$(pwd)
export DARKNET_DIR=$CUR_DIR/darknet
mkdir -p $DARKNET_DIR
cd $DARKNET_DIR
wget -O ${MODEL_NAME}.cfg https://github.com/pjreddie/darknet/blob/master/cfg/${MODEL_NAME}.cfg?raw=true
wget -O ${MODEL_NAME}.weights https://pjreddie.com/media/files/${MODEL_NAME}.weights?raw=true
wget -O libdarknet2.0.so ${REPO_URL}lib/libdarknet2.0.so?raw=true
wget -O coco.names ${REPO_URL}data/coco.names?raw=true
wget -O arial.ttf ${REPO_URL}data/arial.ttf?raw=true
wget -O dog.jpg ${REPO_URL}data/dog.jpg?raw=true
wget -O eagle.jpg ${REPO_URL}data/eagle.jpg?raw=true
wget -O giraffe.jpg ${REPO_URL}data/giraffe.jpg?raw=true
wget -O horses.jpg ${REPO_URL}data/horses.jpg?raw=true
wget -O kite.jpg ${REPO_URL}data/kite.jpg?raw=true
wget -O person.jpg ${REPO_URL}data/person.jpg?raw=true
wget -O scream.jpg ${REPO_URL}data/scream.jpg?raw=true
cd -
cp darknet_tar/* $DARKNET_DIR
# python3 ./deploy_detection-compile_lib.py $DARKNET_DIR
