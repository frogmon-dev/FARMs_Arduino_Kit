# 스마트팜용 안드로이드 킷트

1. flowercare센서 정보 수집장치 (flowercareSensorReciver)
    - 샤오미 flowercare 센서 정보를 bluetooth로 연결하여 mqtt서버로 전송

2. 펌프 onoff 제어장치 (onoffPumpController)
    - mqtt를 통하여 제어명령어를 받아서 펌프의 전원과 연결된 릴레이에 신호를 전달한다.

3. 토양센서 정보 수집장치 (soilSensorReciver)
    - 7insoil 센서의 정보를 수집하여 mqtt 서버로 전송한다. 

4. 하우스 개폐용 모터 제어장치 (updownMotoController)
    - mqtt를 통하여 제어명령어를 받아서 모터 컨트롤러로 신호를 전달한다. 
