#pragma once
#include <memory>

using LazyTreeNodeStatePtr = std::shared_ptr<class LazyTreeNodeState>;

class LazyTreeNodeState {
public:
	bool dummy() const;
	void setDummy(bool set);
	bool temporary() const;
	void setTemporary(bool set);
	bool needFetchChildren() const;
	void setNeedFetchChildren(bool set);

public:
	bool _isDummy = false;
	bool _isTemporary = false;
	bool _needFetchChildren = false;
};
