# TransportCatalogue

TransportCatalogue - система обработки и хранения транспортных маршрутов. Работает через потоки ввода-ввывода, считывает запросы и выдает результат в формате JSON-объекта.

Основные функции:
 - создание базы транспортного справочника и её сериализация в файл;
 - десериализация базы из файла;
 - получение информации о маршруте;
 - получение информации об остановке;
 - поиск оптимального маршрута между двумя остановками;
 - визуализация карты маршрутов.

## Сборка

### Сборка protobuf

1.  Создать папку, в которой разместится пакет Protobuf (`/path/to/protobuf/package`).

2. Скачать protobuf с [репозитория на GitHub](https://github.com/protocolbuffers/protobuf/releases/tag/v3.19.4)

3. Собрать protobuf из исходников и установить
    ```
    cmake <путь к protobuf>/cmake -DCMAKE_BUILD_TYPE=Release \
          -Dprotobuf_BUILD_TESTS=OFF \
          -DCMAKE_INSTALL_PREFIX=/path/to/protobuf/package
    cmake --build .
    cmake --install . 
    ```

### Сборка программы
Перейти в каталог проекта, выполнить:
```
mkdir build && cd "$_"
cmake -DCMAKE_PREFIX_PATH=/home/pi/protobuf/pkg \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF ..
cmake --build .
```

## Запуск

Программа transport_catalogue разделена на две подпрограммы:
1. Программа make_base: создание базы транспортного справочника по запросам base_requests и её сериализация в файл.
2. Программа process_requests: десериализация базы из файла и использование её для ответов на запросы stat_requests. 

Пример запуска программы для заполнения базы:
```
transport_catalogue make_base < make_base.json
```

Пример запуска программы для выполнения запросов к базе:
```
transport_catalogue process_requests < process_requests.json > out.json
```

## Список запросов

### **Запросы на заполнение базы транспортного справочника (make_base)**

Запрос вида make_base имеет следующую структуру:  
```
{
  "base_requests": [ ... ],
  "render_settings": { ... },
  "routing_settings": { ... },
  "serialization_settings": { ... },
  "stat_requests": [ ... ]
}
```  
где:  
`base_requests` — массив с описанием автобусных маршрутов и остановок.  
`stat_requests` — массив с запросами к транспортному справочнику.  
`render_settings` — словарь, содержащий параметры рендеринга карты маршрутов.  
`routing_settings` — словарь, содержащий настройки маршрутов (скорость передвижения и время ожидания на остановке).  
`serialization_settings` — настройки сериализации.

### **Запросы к базе транспортного справочника (process_requests)**

#### Запрос на получение информации об автобусном маршруте:
```
{
  "id": 12345678,
  "type": "Bus",
  "name": "14"
} 
``` 
Ответ на запрос:
```
{
  "curvature": 2.18604,
  "request_id": 12345678,
  "route_length": 9300,
  "stop_count": 4,
  "unique_stop_count": 3
} 
```
В словаре содержатся ключи:
- `curvature` — число типа double, задающее извилистость маршрута. Извилистость равна отношению длины дорожного расстояния маршрута к длине географического расстояния;  
- `request_id` — целое число, равное `id` соответствующего запроса `Bus`;  
- `route_length` — целое число, равное длине маршрута в метрах;  
- `stop_count` — количество остановок на маршруте;  
- `unique_stop_count` — количество уникальных остановок на маршруте.
---
#### Запрос на получение информации об автобусной остановке:
```
{
  "id": 12345,
  "type": "Stop",
  "name": "Улица Докучаева"
} 
```
Ответ на запрос:
```
{
  "buses": [
      "14", "22к"
  ],
  "request_id": 12345
} 
```
---
#### Запрос на получение изображения:
```
{
  "type": "Map",
  "id": 11111
}
```
Ответ на запрос:
```
{
  "map": "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">...\n</svg>",
  "request_id": 11111
} 
```
Ключ `map` — строка с изображением карты в формате `SVG`

---
### Запрос на построение маршрута между двумя остановками
```
{
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Universam",
      "id": 4
}
```
Ответ на запрос:
```
{
          "items": [
              {
                  "stop_name": "Biryulyovo Zapadnoye",
                  "time": 6,
                  "type": "Wait"
              },
              {
                  "bus": "297",
                  "span_count": 2,
                  "time": 5.235,
                  "type": "Bus"
              },
              {
                  "stop_name": "Universam",
                  "time": 6,
                  "type": "Wait"
              },
              {
                  "bus": "635",
                  "span_count": 1,
                  "time": 6.975,
                  "type": "Bus"
              }
          ],
          "request_id": 5,
          "total_time": 24.21
      }
 ```