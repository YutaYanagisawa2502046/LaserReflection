#include "LaserTarget.h"

LaserTarget::LaserTarget(VECTOR position, float radius, LaserColor requiredColor)
	: m_position(position), m_radius(radius), m_isHit(false),m_requiredColor(requiredColor) {
}

void LaserTarget::ResetHitState() {
	m_isHit = false; // 判定をクリア（レーザー側で毎回チェックするため）
}

void LaserTarget::SetHit(LaserColor laserColor, bool hasFilter)
{
	// 💡 レーザーの色と、フィルターの有無をもとに、的が起動するかどうかを判断します
	if (laserColor == m_requiredColor && hasFilter) {
		m_isHit = true;	// 要求通りの色で、フィルターも通過しているなら的が起動します
	}
	else if (!hasFilter)
	{
		m_isHit = true;	// フィルターがない場合は、当たったかで判定します（これでフィルターなしのステージも作れます）
	}
}


void LaserTarget::Draw(bool hasFilter) {
	// 的の描画処理：状態（m_isHit）によって色や見た目を変える
	int x = (int)m_position.x;
	int y = (int)m_position.y;
	int r = (int)m_radius;

	// ベースとなる的の色（要求されている色）
	unsigned int baseColor = GetColor(150, 0, 0);
	// 光っているときの色（要求されている色をベースに、明るく派手な色にする）
	unsigned int brightColor = GetColor(255, 255, 0);

	if (m_requiredColor == LaserColor::Green)
	{
		// 緑が要求されている場合は、ベースは暗めの緑、光るときは明るいシアンっぽい色にします
		baseColor = GetColor(0, 150, 0);
		brightColor = GetColor(255, 0, 255);
	}
	else if (m_requiredColor == LaserColor::Blue) {
		// 青が要求されている場合は、ベースは暗めの青、光るときは明るいシアンっぽい色にします
		baseColor = GetColor(0, 0, 150);
		brightColor = GetColor(0, 255, 255);
	}

	if (m_isHit) {
		// 光っている状態（要求通りの色で起動した！）
		DrawCircle(x, y, r + 5, brightColor, FALSE);
		DrawCircle(x, y, r, brightColor, TRUE);
		DrawCircle(x, y, r - 5, GetColor(255, 255, 255), TRUE); // 中心は白く輝く
		DrawString(x - 20, y - 6, "HIT!!", GetColor(0, 0, 0));
	}
	else {
		// 通常状態（まだ起動していない。要求色が外枠や中心にうっすら見える）
		DrawCircle(x, y, r, baseColor, TRUE);
		DrawCircle(x, y, r, GetColor(255, 255, 255), FALSE);
		DrawCircle(x, y, r - 8, GetColor(255, 255, 255), FALSE);
		DrawCircle(x, y, 4, GetColor(255, 255, 255), TRUE);

		if (hasFilter)
			// デバッグ用兼親切心：要求色を文字で小さく書いておく
			if (m_requiredColor == LaserColor::Blue) {
				DrawString(x - 12, y + r + 5, "NEED BLUE", GetColor(100, 100, 255));
			}
			else if (m_requiredColor == LaserColor::Green) {
				DrawString(x - 12, y + r + 5, "NEED GREEN", GetColor(100, 255, 100));
			}
			else if (m_requiredColor == LaserColor::Red) {
				DrawString(x - 12, y + r + 5, "NEED RED", GetColor(255, 100, 100));
			}
	}
}