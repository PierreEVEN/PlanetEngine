/*******************************************************************************
* EasyCppOGL:   Copyright (C) 2019,                                            *
* Sylvain Thery, IGG Group, ICube, University of Strasbourg, France            *
*                                                                              *
* This library is free software; you can redistribute it and/or modify it      *
* under the terms of the GNU Lesser General Public License as published by the *
* Free Software Foundation; either version 2.1 of the License, or (at your     *
* option) any later version.                                                   *
*                                                                              *
* This library is distributed in the hope that it will be useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this library; if not, write to the Free Software Foundation,      *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
*                                                                              *
* Contact information: thery@unistra.fr                                        *
*******************************************************************************/


#ifndef EASY_CPP_OGL_SG_H_
#define EASY_CPP_OGL_SG_H_

#include <gl_eigen.h>
#include <boundingbox.h>
#include <memory>
#include <tuple>
#include <vector>

namespace EZCOGL
{

class Mesh;

template <typename NODE_TYPE>
class SceneGraphNode
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	using SP = std::shared_ptr<SceneGraphNode>;
	int id_;

protected:
	GLMat4 transfo_;
	BoundingBox::SP bb_;
	std::vector<NODE_TYPE> data_;
	std::vector<SceneGraphNode::SP> children_;
	
	static int next_id;
	SceneGraphNode(const GLMat4& trf) : transfo_(trf), id_(next_id++)
	{
		bb_ = BoundingBox::create();
	}

	SceneGraphNode() : id_(next_id++)
	{
		transfo_.setIdentity();
		bb_ = BoundingBox::create();
	}

	template <typename FUNC>
	inline void traverse_rec_data( const FUNC& f, const GLMat4& trf_parent) const
	{
		alignas(32) GLMat4 trf = transfo_ * trf_parent;

		for (const auto& m : data_)
			f(*(m), trf);
		for (const auto c : children_)
			c->traverse_rec_data( f, trf);
	}


	template <typename FUNC>
	inline void traverse_rec_node(const FUNC& f, const GLMat4& trf_parent) 
	{
		alignas(32) GLMat4 trf = trf_parent * transfo_;
		f(*this, trf);
		for (const auto c : children_)
			c->traverse_rec_node( f, trf);
	}

public:
	inline void clear()
	{
		transfo_.setIdentity();
		data_.clear();
		children_.clear();
	}

	inline static std::shared_ptr<SceneGraphNode> create(const GLMat4& trf)
	{
		return std::shared_ptr<SceneGraphNode>(new SceneGraphNode(trf));
	}

	inline static std::shared_ptr<SceneGraphNode> create()
	{
		return std::shared_ptr<SceneGraphNode>(new SceneGraphNode());
	}

	inline const GLMat4& transfo() const
	{
		return transfo_;
	}

	inline BoundingBox::SP BB()
	{
		return bb_;
	}

//	inline const BoundingBox::SP BB() const
//	{
//		return *bb_;
//	}

	inline const std::vector<NODE_TYPE>& data() const
	{
		return data_;
	}

	inline const std::vector<SceneGraphNode::SP>& children() const
	{
		return children_;
	}

	void add_child(SceneGraphNode::SP node)
	{
		children_.push_back(node);
	}

	inline void add_data(NODE_TYPE d)
	{
		data_.push_back(std::move(d));
	}

	inline void reserve_children(int n)
	{
		children_.reserve(n);
	}

	inline void reserve_data(int n)
	{
		data_.reserve(n);
	}
	
	// f(const NODE_TYPE&, const GLMat4&)
	template<typename FUNC>
	inline void traverse_data(const FUNC& f) const
	{
		GLMat4 trf;
		trf.setIdentity();
		traverse_rec_data( f, trf);
	}

	// f(const SceneGraphNode&, const GLMat4&)
	template <typename FUNC>
	inline void traverse_node(const FUNC& f) 
	{
		GLMat4 trf;
		trf.setIdentity();
		traverse_rec_node(f, trf);
	}

	template <typename FUNC>
	inline void flatten(const FUNC& f) 
	{
		decltype(data_) tempo;
		traverse_node([&tempo,&f,this](SceneGraphNode& node, const GLMat4& trf) {
			for (auto d : node.data_)
			{
				f(*d, trf);
				tempo.push_back(d);
			}
		});
		children_.clear();
		tempo.swap(data_);
		transfo_.setIdentity();
	}


	void dump(int depth)
	{
		std::cout << "ID:" << id_ << "  Depth:" << depth << "  Data:" << data_.size()
			<< "  Children:" << children_.size() << std::endl;
		std::cout << " [ ";
		for (auto c : children_)
			std::cout << c->id_ << ", ";
		std::cout << " ] " << std::endl;
		for (auto c : children_)
			c->dump(depth + 1);
	}
};


}
#endif
