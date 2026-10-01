//---------------------------------------------------------------------------
#ifndef Parse_treeH
#define Parse_treeH

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <QString>
#include <QIODevice>
#include <memory>
#include <vector>

#include "NodeTypes.h"

//---------------------------------------------------------------------------
class tree {
private:
    QString    value;
    node_type  type;
    int        num_subnode;   // количество подчинённых

    tree* parent;             // +1
    tree* next;               // 0
    tree* prev;               // 0
    tree* first;              // -1
    tree* last;               // -1
    unsigned int index;

public:
    tree(const QString& _value, const node_type _type, tree* _parent);
    ~tree();

    tree* add_child(const QString& _value, const node_type _type);
    tree* add_child();
    tree* add_node();

    QString&   get_value();
    node_type  get_type();
    int        get_num_subnode();

    tree* get_subnode(int _index);
    tree* get_subnode(const QString& node_name);
    tree* get_next();
    tree* get_parent();
    tree* get_first();
    tree* get_last();

    tree& operator[](int _index);

    void set_value(const QString& v, const node_type t);

    void outtext(QString& text);

    QString path();
};

typedef tree* treeptr;
typedef std::unique_ptr<tree> tree_unique_ptr;
typedef std::shared_ptr<tree> tree_shared_ptr;

//---------------------------------------------------------------------------
tree* parse_1Ctext(const QString& text, const QString& path);
tree* parse_1Cstream(QIODevice* str, const QString& path);
bool  test_parse_1Ctext(QIODevice* str, const QString& path);
QString outtext(tree* t);
tree* find_node_by_guid(tree* root, const QString& target_guid);
tree* find_metadata_node_by_guid(tree* root, const QString& target_guid);

#endif