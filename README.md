
# Поисковая система
Основные функции поисковой системы включают в себя:

-  Ранжирование результатов поиска с использованием TF-IDF.
-  Создание и обработка очереди запросов.
-  Обработка минус-слов (документы, содержащие такие минус-слова, не включаются в результаты поиска).
-  Обработка стоп-слов (которые не учитываются системой и не влияют на результаты поиска).
-  Удаление дубликатов документов.
-  Возможность работы в параллельном режиме.
-  Разбиение результатов поиска на страницы.

## Использование

 1. Создание экземпляра класса SearchServer, в конструктор которого передается строка со стоп-словами, разделенными пробелами, или другой контейнер с последовательным доступом к элементам для использования в циклах for-range.

2. Добавление документов для поиска с помощью метода AddDocument. В метод передаются id документа, статус, рейтинг и сам документ в виде строки.

3. Метод FindTopDocuments возвращает вектор документов, соответствующих ключевым словам. Результаты сортируются по TF-IDF. Есть возможность дополнительной фильтрации документов по id, статусу и рейтингу. Метод доступен как в однопоточной, так и в многопоточной версии.

### Пример использования
<details>  
<summary>Запрос</summary>

```cpp
void PrintDocument(const Document &document) {  
    cout << "{ "s  
         << "document_id = "s << document.id << ", "s  
         << "relevance = "s << document.relevance << ", "s  
         << "rating = "s << document.rating << " }"s << endl;  
}  
  
int main() {  
    SearchServer search_server("and with"s);  
    int id = 0;  
    for (  
        const string &text: {  
  "white cat and yellow hat"s,  
            "curly cat curly tail"s,  
            "nasty dog with big eyes"s,  
            "nasty pigeon john"s,  
    }  
  ) {  
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});  
    }  
    cout << "ACTUAL by default:"s << endl;  
    // последовательная версия  
  for (const Document &document: search_server.FindTopDocuments("curly nasty cat"s)) {  
        PrintDocument(document);  
    }  
    cout << "BANNED:"s << endl;  
    // последовательная версия  
  for (const Document &document: search_server.FindTopDocuments(execution::seq, "curly nasty cat"s,  
                                                                  DocumentStatus::BANNED)) {  
        PrintDocument(document);  
    }  
    cout << "Even ids:"s << endl;  
    // параллельная версия  
  for (const Document &document: search_server.FindTopDocuments(execution::par, "curly nasty cat"s,  
                                                                  [](int document_id, DocumentStatus status,  
                                                                     int rating) { return document_id % 2 == 0; })) {  
        PrintDocument(document);  
    }  
    return 0;  
}  
```
</details>





<details>
<summary> Вывод </summary>

```bash
ACTUAL by default:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
{ document_id = 1, relevance = 0.173287, rating = 1 }
{ document_id = 3, relevance = 0.173287, rating = 1 }
BANNED:
Even ids:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
```
</details>

## Установка
Для установки необходимо скомпилировать программу в любой IDE или через консоль.

## Требования
C++ 17 и выше.
