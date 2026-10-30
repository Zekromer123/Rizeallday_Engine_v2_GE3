#include "NavGrid.h"
#include <algorithm>
#include <cmath>
#include <queue>

void NavGrid::Build(float minX, float minZ, float maxX, float maxZ, float cellSize)
{
	cellSize = (std::max)(cellSize, 0.05f);
	const float sizeX = (std::max)(maxX - minX, cellSize);
	const float sizeZ = (std::max)(maxZ - minZ, cellSize);
	while ((sizeX / cellSize) * (sizeZ / cellSize) > static_cast<float>(kMaxCells)) cellSize *= 1.5f;
	minX_ = minX;
	minZ_ = minZ;
	cellSize_ = cellSize;
	width_ = static_cast<int>(std::ceil(sizeX / cellSize));
	depth_ = static_cast<int>(std::ceil(sizeZ / cellSize));
	blocked_.assign(static_cast<size_t>(width_) * depth_, 0);
}

void NavGrid::BlockRect(float minX, float minZ, float maxX, float maxZ)
{
	// Cells whose centre is inside: centre of cell c is minX_ + (c + 0.5) * cellSize_
	const int x0 = (std::max)(0, static_cast<int>(std::ceil((minX - minX_) / cellSize_ - 0.5f)));
	const int x1 = (std::min)(width_ - 1, static_cast<int>(std::floor((maxX - minX_) / cellSize_ - 0.5f)));
	const int z0 = (std::max)(0, static_cast<int>(std::ceil((minZ - minZ_) / cellSize_ - 0.5f)));
	const int z1 = (std::min)(depth_ - 1, static_cast<int>(std::floor((maxZ - minZ_) / cellSize_ - 0.5f)));
	for (int z = z0; z <= z1; ++z) {
		for (int x = x0; x <= x1; ++x) blocked_[static_cast<size_t>(z) * width_ + x] = 1;
	}
}

bool NavGrid::CellOf(float x, float z, int& cx, int& cz) const
{
	cx = static_cast<int>(std::floor((x - minX_) / cellSize_));
	cz = static_cast<int>(std::floor((z - minZ_) / cellSize_));
	return cx >= 0 && cz >= 0 && cx < width_ && cz < depth_;
}

bool NavGrid::IsBlockedAt(float x, float z) const
{
	int cx = 0, cz = 0;
	return !CellOf(x, z, cx, cz) || blocked_[static_cast<size_t>(cz) * width_ + cx] != 0;
}

Vector3 NavGrid::CellCentre(int cx, int cz, float y) const
{
	return { minX_ + (cx + 0.5f) * cellSize_, y, minZ_ + (cz + 0.5f) * cellSize_ };
}

bool NavGrid::NearestFree(int& cx, int& cz) const
{
	if (Free(cx, cz)) return true;
	for (int r = 1; r <= 3; ++r) {
		int bestX = -1, bestZ = -1, bestD = 1 << 30;
		for (int dz = -r; dz <= r; ++dz) {
			for (int dx = -r; dx <= r; ++dx) {
				const int d = dx * dx + dz * dz;
				if (Free(cx + dx, cz + dz) && d < bestD) { bestD = d; bestX = cx + dx; bestZ = cz + dz; }
			}
		}
		if (bestX >= 0) { cx = bestX; cz = bestZ; return true; }
	}
	return false;
}

bool NavGrid::LineIsFree(const Vector3& a, const Vector3& b) const
{
	const float dx = b.x - a.x, dz = b.z - a.z;
	const int samples = (std::max)(1, static_cast<int>(std::ceil(std::sqrt(dx * dx + dz * dz) / (cellSize_ * 0.25f))));
	for (int i = 0; i <= samples; ++i) {
		const float t = static_cast<float>(i) / samples;
		if (IsBlockedAt(a.x + dx * t, a.z + dz * t)) return false;
	}
	return true;
}

std::vector<Vector3> NavGrid::FindPath(const Vector3& from, const Vector3& to) const
{
	int sx = 0, sz = 0, gx = 0, gz = 0;
	if (IsEmpty() || !CellOf(from.x, from.z, sx, sz) || !CellOf(to.x, to.z, gx, gz)) return {};
	const bool startSnapped = !Free(sx, sz);
	const bool goalSnapped = !Free(gx, gz);
	if (!NearestFree(sx, sz) || !NearestFree(gx, gz)) return {};

	// A* with the octile distance; diagonals only when both side cells are free (no corner cutting)
	const int count = width_ * depth_;
	std::vector<float> cost(static_cast<size_t>(count), 1e30f);
	std::vector<int> parent(static_cast<size_t>(count), -1);
	auto heuristic = [&](int x, int z) {
		const float ax = static_cast<float>(std::abs(x - gx)), az = static_cast<float>(std::abs(z - gz));
		return (ax + az) + (1.41421356f - 2.0f) * (std::min)(ax, az);
	};
	using Item = std::pair<float, int>; // f, cell
	std::priority_queue<Item, std::vector<Item>, std::greater<Item>> open;
	const int start = sz * width_ + sx;
	const int goal = gz * width_ + gx;
	cost[start] = 0.0f;
	open.push({ heuristic(sx, sz), start });
	while (!open.empty()) {
		const auto [f, cell] = open.top();
		open.pop();
		if (cell == goal) break;
		const int x = cell % width_, z = cell / width_;
		if (f - heuristic(x, z) > cost[cell] + 1e-4f) continue; // stale entry
		for (int dz = -1; dz <= 1; ++dz) {
			for (int dx = -1; dx <= 1; ++dx) {
				if ((dx == 0 && dz == 0) || !Free(x + dx, z + dz)) continue;
				if (dx != 0 && dz != 0 && (!Free(x + dx, z) || !Free(x, z + dz))) continue;
				const int next = (z + dz) * width_ + (x + dx);
				const float g = cost[cell] + ((dx != 0 && dz != 0) ? 1.41421356f : 1.0f);
				if (g < cost[next]) {
					cost[next] = g;
					parent[next] = cell;
					open.push({ g + heuristic(x + dx, z + dz), next });
				}
			}
		}
	}
	if (cost[goal] >= 1e30f) return {};

	// Cell chain -> points (exact start / goal unless they were snapped out of an obstacle)
	std::vector<Vector3> raw;
	for (int c = goal; c != -1; c = parent[c]) raw.push_back(CellCentre(c % width_, c / width_, from.y));
	std::reverse(raw.begin(), raw.end());
	raw.front() = startSnapped ? raw.front() : Vector3{ from.x, from.y, from.z };
	raw.back() = goalSnapped ? raw.back() : Vector3{ to.x, from.y, to.z };

	// Keep only the corners: from each kept point, jump to the farthest point still in straight sight
	std::vector<Vector3> path{ raw.front() };
	size_t anchor = 0;
	while (anchor + 1 < raw.size()) {
		size_t next = anchor + 1;
		for (size_t j = raw.size() - 1; j > anchor + 1; --j) {
			if (LineIsFree(raw[anchor], raw[j])) { next = j; break; }
		}
		path.push_back(raw[next]);
		anchor = next;
	}
	return path;
}
