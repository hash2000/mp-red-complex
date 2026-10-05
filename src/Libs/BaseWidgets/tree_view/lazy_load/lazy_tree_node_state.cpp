#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_state.h"

bool LazyTreeNodeState::dummy() const {
	return _isDummy;
}

void LazyTreeNodeState::setDummy(bool set) {
	_isDummy = set;
}

bool LazyTreeNodeState::temporary() const {
	return _isTemporary;
}

void LazyTreeNodeState::setTemporary(bool set) {
	_isTemporary = set;
}

bool LazyTreeNodeState::needFetchChildren() const {
	return _needFetchChildren;
}

void LazyTreeNodeState::setNeedFetchChildren(bool set) {
	_needFetchChildren = set;
}

