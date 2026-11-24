#include "guis/GuiInfoPopup.h"

#include "components/ComponentGrid.h"
#include "components/ImageComponent.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include <SDL_timer.h>
#include "TextToSpeech.h"

GuiInfoPopup::GuiInfoPopup(Window* window, std::string message, int duration) :
	GuiComponent(window), mMessage(message), mDuration(duration), mRunning(true), mFadeOut(0),
	mGrid(nullptr), mFrame(nullptr), mThemed(false), mBackgroundColor(0), mBorderColor(0), mCornerRadius(0),
	mSticky(false), mPercent(-1), mRowHeight(0)
{
	auto theme = ThemeData::getMenuTheme();

	TextToSpeech::getInstance()->say(message, true);

	// we only init the actual time when we first start to render
	mStartTime = 0;

	const Window::PopupStyle& style = window->getNoticeStyle();
	if (style.enabled && style.font)
	{
		// The theme's box: as tall as its style, as wide as the text needs up to the style's width; the icon, then
		// the text on one line (cut with an ellipsis). The window places it.
		mThemed = true;
		mBackgroundColor = style.backgroundColor;
		mBorderColor = style.borderColor;
		mCornerRadius = style.cornerRadius;

		float height = style.size.y();
		float padding = Math::round(height * 0.4f);
		float gap = Math::round(height * 0.28f);
		float icon = style.iconPath.empty() ? 0.0f : (style.iconSize > 0 ? style.iconSize : Math::round(height * 0.5f));
		float textX = padding + (icon > 0 ? icon + gap : 0.0f);
		float textWidth = Math::max(0.0f, Math::min(style.font->sizeText(message).x(), style.size.x() - textX - padding));

		if (icon > 0)
		{
			mIcon = std::make_shared<ImageComponent>(window);
			mIcon->setImage(style.iconPath);
			mIcon->setOrigin(0.5f, 0.5f);
			mIcon->setMaxSize(icon, icon);
			mIcon->setPosition(padding + icon / 2.0f, height / 2.0f);
			addChild(mIcon.get());
		}

		mText = std::make_shared<TextComponent>(window, message, style.font, style.color, ALIGN_LEFT);
		mText->setMultiLine(TextComponent::MultiLineType::SINGLELINE);
		mText->setPosition(textX, 0);
		mText->setSize(textWidth, height);
		addChild(mText.get());

		mSize = Vector2f(textX + textWidth + padding, height);
		mRowHeight = height;
		return;
	}

	mFrame = new NinePatchComponent(window);
	float maxWidth = Renderer::getScreenWidth() * 0.9f;
	float maxHeight = Renderer::getScreenHeight() * 0.2f;

	std::shared_ptr<TextComponent> s = std::make_shared<TextComponent>(mWindow,
		"",
		Font::get(FONT_SIZE_MINI),
		theme->Text.color,
		ALIGN_CENTER);

	// we do this to force the text container to resize and return an actual expected popup size
	s->setSize(0,0);
	s->setText(message);
	mSize = s->getSize();

	// confirm the size isn't larger than the screen width, otherwise cap it
	if (mSize.x() > maxWidth) {
		s->setSize(maxWidth, mSize[1]);
		mSize[0] = maxWidth;
	}
	if (mSize.y() > maxHeight) {
		s->setSize(mSize[0], maxHeight);
		mSize[1] = maxHeight;
	}

	// add a padding to the box
	int paddingX = (int) (Renderer::getScreenWidth() * 0.03f);
	int paddingY = (int) (Renderer::getScreenHeight() * 0.02f);
	mSize[0] = mSize.x() + paddingX;
	mSize[1] = mSize.y() + paddingY;

	float posX = Renderer::getScreenWidth()*0.5f - mSize.x()*0.5f;
	float posY = Renderer::getScreenHeight() * 0.02f;

	setPosition(posX, posY, 0);

	mFrame->setImagePath(theme->Background.path);
	mFrame->setEdgeColor(theme->Background.color);
	mFrame->setCenterColor(theme->Background.centerColor);
	mFrame->setCornerSize(theme->Background.cornerSize);
	mFrame->setPostProcessShader(theme->Background.menuShader, false);
	mFrame->fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	addChild(mFrame);

	mGrid = new ComponentGrid(window, Vector2i(1, 3));
	mGrid->setSize(mSize);
	mGrid->setEntry(s, Vector2i(0, 1), false, true);
	addChild(mGrid);
}

void GuiInfoPopup::render(const Transform4x4f& /*parentTrans*/)
{
	if (!mRunning)
		return;

	// we use identity as we want to render on a specific window position, not on the view
	Transform4x4f trans = getTransform() * Transform4x4f::Identity();

	Renderer::setMatrix(trans);

	if (mThemed)
	{
		auto withOpacity = [this](unsigned int color) { return (color & 0xFFFFFF00) | ((color & 0xFF) * getOpacity() / 255); };
		Renderer::drawSolidRectangle(0, 0, mSize.x(), mSize.y(), withOpacity(mBackgroundColor), withOpacity(mBorderColor),
			(mBorderColor & 0xFF) ? 1.0f : 0.0f, mCornerRadius);

		if (mPercent >= 0 && mText != nullptr)
		{
			float x = mText->getPosition().x();
			float w = mSize.x() - x - Math::round(mRowHeight * 0.4f);
			float h = Math::max(2.0f, Math::round(mRowHeight * 0.12f));
			float y = mSize.y() - h - Math::round(mRowHeight * 0.26f);
			unsigned int ink = mText->getColor();

			Renderer::drawRect(x, y, w, h, withOpacity((ink & 0xFFFFFF00) | 0x38));
			Renderer::drawRect(x, y, w * Math::min(mPercent, 100) / 100.0f, h, withOpacity(ink));
		}
	}

	renderChildren(trans);	
}

// In a theme's style the box takes the style's full width, so it does not change size with its text: the title
// beside the icon, the detail under it and a gauge along the bottom
void GuiInfoPopup::setProgress(const std::string& title, const std::string& detail, int percent)
{
	mSticky = true;
	mPercent = percent;
	mMessage = title;

	if (!mThemed || mText == nullptr)
		return;

	const Window::PopupStyle& style = mWindow->getNoticeStyle();
	float textX = mText->getPosition().x();
	float textWidth = Math::max(0.0f, style.size.x() - textX - Math::round(mRowHeight * 0.4f));

	mText->setText(title);
	mText->setSize(textWidth, mRowHeight);

	if (mDetail == nullptr)
	{
		mDetail = std::make_shared<TextComponent>(mWindow, detail, style.font, style.color, ALIGN_LEFT);
		mDetail->setMultiLine(TextComponent::MultiLineType::SINGLELINE);
		mDetail->setPosition(textX, Math::round(mRowHeight * 0.78f));
		addChild(mDetail.get());
	}

	mDetail->setText(detail);
	mDetail->setSize(textWidth, mRowHeight);

	mSize = Vector2f(style.size.x(), Math::round(mRowHeight * 2.2f));
}

void GuiInfoPopup::finish()
{
	mSticky = false;

	if (mStartTime == 0)
		mRunning = false;
	else
		mDuration = SDL_GetTicks() - mStartTime + 250;
}

void GuiInfoPopup::shorten(int ms)
{
	if (!mRunning || mStartTime == 0)
		return;

	int elapsed = SDL_GetTicks() - mStartTime;
	if (mDuration - elapsed > ms)
		mDuration = elapsed + ms;
}

void GuiInfoPopup::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	int curTime = SDL_GetTicks();

	// we only init the actual time when we first start to render
	if (mStartTime == 0)
		mStartTime = curTime;
	else if (mSticky && curTime - mStartTime > 250)
	{
		// A job's progress stays as it is until the job finishes it
		setOpacity(255);
		return;
	}
	else if (!mSticky && (curTime - mStartTime > mDuration || curTime < mStartTime))
	{
		// If we're past the popup duration, no need to render or if SDL reset : Stop
		mRunning = false;
		mFadeOut = 1.0;
		return;
	}

	#define FADE_DURATION 250

	// compute fade in effect
	int alpha = 255;

	if (curTime - mStartTime <= FADE_DURATION)
		alpha = ((curTime - mStartTime) * 255 / FADE_DURATION);
	else if (curTime - mStartTime >= mDuration - FADE_DURATION)
	{
		alpha = ((-(curTime - mStartTime - mDuration) * 255) / FADE_DURATION);
		mFadeOut = (float) (255 - alpha) / 255.0;
	}

	alpha = (int) (Math::easeOutCubic(alpha / 255.0f) * 255.0f);

	setOpacity((unsigned char)alpha);

	// a theme's box fades in place
	if (!mThemed)
		setScale(alpha / 255.0f);
}
