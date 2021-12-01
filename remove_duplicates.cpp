#include "remove_duplicates.h"

#include <algorithm>
#include <set>
#include <iostream>
#include "search_server.h"

std::set<std::string_view> wordSplit(int doc_ID, const SearchServer& search_server) {
    std::set<std::string_view> Words;		/// придерживайтесь соглашения об названии, с подчеркиванием только поля классов
    for (const auto& [word, _] : search_server.GetWordFrequencies(doc_ID))		
        Words.insert(word);
    return Words;
}

/// не замечание, но можете поработать еще над данной функцией, улучшить, если не получится, то можете оставить как есть.
/// в таком виде у вас сложность будет выше чем заявлено в задании
/// для эттого предлагаю:
/// 1. поменять тип контейнера remove_ID, у вас есть поиск по данному контейнеру, к примеру, у std::set лучше характиристика поиска, и он уже отсортированный, т.е. можно избавиться от сортировки ниже
/// 2. у вас 2-йно проход по контейнеру и получается, что сравниваете словарь слов каждый с каждым, при этом этот словарь каждый раз создаете.
///    можно завести дополнительный контейнер (std::set или std::map) для словарей которые уже были
///    получается один проход по search_server с проверкой словаря в этом контейнере, если его там нет, то добавляем, если есть, то ID для удаления

/// Как реализовать с одним циклом не придумал, 
void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> remove_ID;

    for (auto iterator_first = search_server.begin(); iterator_first != search_server.end(); ++iterator_first)
        if(remove_ID.count(*iterator_first)==0) {			
            std::set<std::string>   words_first = wordSplit(*iterator_first, search_server);	
            for (auto iterator_second = std::next(iterator_first); iterator_second != search_server.end(); ++iterator_second)
                if (remove_ID.count(*iterator_second) == 0) {	
                    std::set<std::string>  words_second = wordSplit(*iterator_second, search_server);
                    if (words_first == words_second) remove_ID.insert(*iterator_second);
            }
     }
    for (int delet_ID : remove_ID) {
        search_server.RemoveDocument(delet_ID);
        std::cout << "Found duplicate document id " << delet_ID << std::endl;
    }
}
