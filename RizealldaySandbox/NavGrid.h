#pragma once
#include "math/Vector3.h"
#include <cstdint>
#include <vector>

/// Walkable grid on the XZ plane for A* pathfinding. Pure logic: the editor fills it from static colliders.
class NavGrid {
public:
	/// All cells free. Very large areas get a coarser cell so the grid stays under kMaxCells.
	void Build(float minX, float minZ, float maxX, float maxZ, float cellSize);
	/// Blocks every cell whose centre lies inside the rectangle (callers inflate it by the agent radius)
	void BlockRect(float minX, float minZ, float maxX, float maxZ);

	bool IsEmpty() const { return width_ == 0 || depth_ == 0; }
	bool IsBlockedAt(float x, float z) const;
	float CellSize() const { return cellSize_; }

	/// 8-connected A* (no corner cutting) from `from` to `to`, shortened to the corners that matter.
	/// Points keep the y of `from`. A start or goal inside an obstacle snaps to the nearest free cell close by.
	/// Empty when there is no path or the points are outside the grid.
	std::vector<Vector3> FindPath(const Vector3& from, const Vector3& to) const;

	static constexpr int kMaxCells = 512 * 512;

private:
	bool CellOf(float x, float z, int& cx, int& cz) const;
	bool Free(int cx, int cz) const { return cx >= 0 && cz >= 0 && cx < width_ && cz < depth_ && !blocked_[cz * width_ + cx]; }
	bool NearestFree(int& cx, int& cz) const;
	bool LineIsFree(const Vector3& a, const Vector3& b) const;
	Vector3 CellCentre(int cx, int cz, float y) const;

	float minX_ = 0.0f, minZ_ = 0.0f, cellSize_ = 1.0f;
	int width_ = 0, depth_ = 0;
	std::vector<uint8_t> blocked_;
};
