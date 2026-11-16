#include "Content/UsersModule/widgets/user_widget.h"
#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/models/user_view.h"
//#include "Content/TexturesModule/services/images_service.h"
#include "Libs/Graphics/textures/extensions/pixmap_extensions.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QTimer>

class UserWidget::Private {
public:
	Private(UserWidget* parent)
		: q(parent) {
	}

	UserWidget* q;
	UsersService* usersService = nullptr;
	//ImagesService* ImagesService = nullptr;
	std::shared_ptr<UserView> currentUser;

	// Элементы заголовка
	QLabel* userIconLabel = nullptr;
	QLabel* userIdLabel = nullptr;
	QLabel* displayNameLabel = nullptr;

	// Контейнер для персонажей
	QVBoxLayout* charactersLayout = nullptr;
	QWidget* charactersContainer = nullptr;
	QScrollArea* scrollArea = nullptr;
};

UserWidget::UserWidget(UsersService* usersService, QWidget* parent)
	: QFrame(parent)
	, d(std::make_unique<Private>(this)) {
	d->usersService = usersService;
	//d->ImagesService = ImagesService;

	setObjectName("UserWidget");
	setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	setLineWidth(1);

	setupLayout();
	loadUserData();
}

UserWidget::~UserWidget() = default;

void UserWidget::resizeEvent(QResizeEvent* event) {
	QFrame::resizeEvent(event);
}

void UserWidget::setupLayout() {
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(8);

	// === Заголовок пользователя ===
	auto* headerFrame = new QFrame(this);
	headerFrame->setObjectName(QStringLiteral("UserHeader"));
	headerFrame->setFrameStyle(QFrame::StyledPanel);

	auto* headerLayout = new QHBoxLayout(headerFrame);
	headerLayout->setSpacing(10);
	headerLayout->setContentsMargins(8, 4, 8, 4);

	// Иконка пользователя (слева)
	d->userIconLabel = new QLabel(headerFrame);
	d->userIconLabel->setObjectName(QStringLiteral("UserIcon"));
	d->userIconLabel->setFixedSize(54, 54);
	d->userIconLabel->setScaledContents(true);
	headerLayout->addWidget(d->userIconLabel, 0, Qt::AlignVCenter);

	// Правая часть заголовка
	auto* infoLayout = new QVBoxLayout();
	infoLayout->setSpacing(2);

	// ID пользователя (серое поле)
	d->userIdLabel = new QLabel(headerFrame);
	d->userIdLabel->setObjectName(QStringLiteral("UserId"));
	d->userIdLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	infoLayout->addWidget(d->userIdLabel);

	// DisplayName
	d->displayNameLabel = new QLabel(headerFrame);
	d->displayNameLabel->setObjectName(QStringLiteral("UserDisplayName"));
	infoLayout->addWidget(d->displayNameLabel);

	infoLayout->addStretch();

	headerLayout->addLayout(infoLayout, 1);

	mainLayout->addWidget(headerFrame, 0);

	// === Область прокрутки для персонажей (без рамки, линия слева) ===
	d->scrollArea = new QScrollArea(this);
	d->scrollArea->setObjectName(QStringLiteral("UserScroll"));
	d->scrollArea->setWidgetResizable(false);  // ВАЖНО: не растягивать контейнер
	d->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	d->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	//// Контейнер для персонажей внутри прокрутки
	//d->charactersContainer = new QWidget();
	//d->charactersContainer->setStyleSheet("background-color: #1a202c;");
	//// Контейнер имеет фиксированную ширину (устанавливается в updateCharactersContainerWidth)
	//// и фиксированную высоту по содержимому
	//d->charactersContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	//
	//d->charactersLayout = new QVBoxLayout(d->charactersContainer);
	//d->charactersLayout->setContentsMargins(4, 4, 4, 4);
	//d->charactersLayout->setSpacing(4);
	//d->charactersLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

	//d->scrollArea->setWidget(d->charactersContainer);

	mainLayout->addWidget(d->scrollArea, 1);
}

void UserWidget::loadUserData() {
	//d->currentUser = d->usersService->currentUser();
	//if (!d->currentUser) {
	//	d->displayNameLabel->setText("Не авторизован");
	//	d->userIdLabel->setText("N/A");
	//	return;
	//}

	//const auto user = d->currentUser.get();

	//// DisplayName
	//d->displayNameLabel->setText(user->data->displayName.isEmpty() ? "Пользователь" : user->data->displayName);

	//// ID (показываем сокращённую версию)
	//QString shortId = user->data->loginHash;
	//if (shortId.length() > 12) {
	//	shortId = shortId.left(12) + "...";
	//}
	//d->userIdLabel->setText(shortId);

	//// Иконка пользователя
	//if (!user->data->icon.isNull()) {
	//	d->userIconLabel->setPixmap(Extensions::Pixmaps::scale(user->data->icon, 48, -1));
	//}
}
