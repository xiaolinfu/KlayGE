// RenderTarget.cpp
// KlayGE 渲染目标类 实现文件
// Ver 2.2.0
// 版权所有(C) 龚敏敏, 2003-2004
// Homepage: http://klayge.sourceforge.net
//
// 2.2.0
// 改用boost::timer计时 (2004.11.1)
//
// 2.1.2
// 简化了UpdateStats (2004.7.19)
//
// 修改记录
//////////////////////////////////////////////////////////////////////////////////

#include <KlayGE/KlayGE.hpp>
#include <KlayGE/Math.hpp>
#include <KlayGE/Viewport.hpp>
#include <KlayGE/SceneManager.hpp>
#include <KlayGE/Context.hpp>

#include <KlayGE/RenderTarget.hpp>

namespace KlayGE
{
	// 构造函数
	/////////////////////////////////////////////////////////////////////////////////
	RenderTarget::RenderTarget()
					: active_(true),
						FPS_(0)
	{
	}

	// 析构函数
	/////////////////////////////////////////////////////////////////////////////////
	RenderTarget::~RenderTarget()
	{
	}

	// 渲染目标的左坐标
	/////////////////////////////////////////////////////////////////////////////////
	int RenderTarget::Left() const
	{
		return left_;
	}

	// 渲染目标的顶坐标
	/////////////////////////////////////////////////////////////////////////////////
	int RenderTarget::Top() const
	{
		return top_;
	}

	// 渲染目标的宽度
	/////////////////////////////////////////////////////////////////////////////////
	int RenderTarget::Width() const
	{
		return width_;
	}

	// 渲染目标的高度
	/////////////////////////////////////////////////////////////////////////////////
	int RenderTarget::Height() const
	{
		return height_;
	}

	// 渲染目标的颜色深度
	/////////////////////////////////////////////////////////////////////////////////
	int RenderTarget::ColorDepth() const
	{
		return colorDepth_;
	}

	// 刷新渲染目标
	/////////////////////////////////////////////////////////////////////////////////
	void RenderTarget::Update()
	{
		Context::Instance().SceneManagerInstance().Update();

		this->UpdateStats();
	}

	// 获取视口
	/////////////////////////////////////////////////////////////////////////////////
	Viewport const & RenderTarget::GetViewport() const
	{
		return viewport_;
	}

	Viewport& RenderTarget::GetViewport()
	{
		return viewport_;
	}

	// 设置视口
	/////////////////////////////////////////////////////////////////////////////////
	void RenderTarget::SetViewport(Viewport const & viewport)
	{
		viewport_ = viewport;
	}

	// 获取渲染目标的每秒帧数
	/////////////////////////////////////////////////////////////////////////////////
	float RenderTarget::FPS() const
	{
		return FPS_;
	}

	// 更新状态
	/////////////////////////////////////////////////////////////////////////////////
	void RenderTarget::UpdateStats()
	{
		static float accumulateTime = 0;
		static long numFrames  = 0;
		
		// measure statistics
		++ numFrames;
		accumulateTime += static_cast<float>(timer_.elapsed());

		// check if new second
		if (accumulateTime > 1)
		{
			// new second - not 100% precise
			FPS_ = static_cast<float>(numFrames) / accumulateTime;

			accumulateTime = 0;
			numFrames  = 0;
		}

		timer_.restart();
	}

	// 获取该渲染目标是否处于活动状态
	/////////////////////////////////////////////////////////////////////////////////
	bool RenderTarget::Active() const
	{
		return active_;
	}
	
	// 设置该渲染目标是否处于活动状态
	/////////////////////////////////////////////////////////////////////////////////
	void RenderTarget::Active(bool state)
	{
		active_ = state;
	}
}        
