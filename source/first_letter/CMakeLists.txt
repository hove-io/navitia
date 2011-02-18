FIND_PACKAGE(Boost 1.40.0 COMPONENTS system REQUIRED)
FIND_PACKAGE(WS_Commons)
#link_directories ( ${Boost_LIBRARY_DIRS} )
#include_directories ( ${Boost_INCLUDE_DIRS} )

MAKE_WS(trie trie.cpp)
#add_executable(trie_test trie.cpp)
target_link_libraries(trie)
