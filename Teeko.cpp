#include "Teeko.h"
#include "ui_Teeko.h"

#include <QDebug>
#include <QMessageBox>
#include <QActionGroup>
#include <QSignalMapper>

Teeko::Teeko(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::Teeko),
      m_player(Player::player(Player::Red)),
      m_phase(Teeko::DropPhase),
      m_redHolesCount(0),
      m_blueHolesCount(0) {

    ui->setupUi(this);

    QObject::connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(reset()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAbout()));
    holeSelected = false;
    lastMove = -1;

    QSignalMapper* map = new QSignalMapper(this);
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            QString holeName = QString("hole%1%2").arg(row).arg(col);
            Hole* hole = this->findChild<Hole*>(holeName);
            Q_ASSERT(hole != nullptr);

            m_board[row][col] = hole;

            int id = row * 5 + col;
            map->setMapping(hole, id);
            QObject::connect(hole, SIGNAL(clicked(bool)), map, SLOT(map()));
        }
    }
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QObject::connect(map, SIGNAL(mapped(int)), this, SLOT(play(int)));
#else
    QObject::connect(map, SIGNAL(mappedInt(int)), this, SLOT(play(int)));
#endif

    // When the turn ends, switch the player.
    QObject::connect(this, SIGNAL(turnEnded()), this, SLOT(switchPlayer()));

    this->reset();

    this->adjustSize();
    this->setFixedSize(this->size());
}

Teeko::~Teeko() {
    delete ui;
}

void Teeko::setPhase(Teeko::Phase phase) {
    if (m_phase != phase) {
        m_phase = phase;
        emit phaseChanged(phase);
    }
}

void Teeko::play(int id) {
    Hole* hole = m_board[id / 5][id % 5];

    if (m_phase == Teeko::DropPhase) {
        // Logic to the DropPhase
        if (m_redHolesCount < 4 || m_blueHolesCount < 4) {
            if (hole->isEmpty()) {
                hole->setPlayer(m_player);
                // Increases the counter of holes after placing one
                if (m_player == Player::player(Player::Red)) {
                    m_redHolesCount++;
                } else {
                    m_blueHolesCount++;
                }

                if ((m_redHolesCount == 4 && m_blueHolesCount == 4) && m_phase == Teeko::DropPhase) {
                    // If hit 4 holes each, change the phase to the MovePhase
                    setPhase(Teeko::MovePhase);
                    updateStatusBar();
                }
                CheckWin(m_player->type());

                emit turnEnded();
            }
        }
    }else if (m_phase == Teeko::MovePhase) {
            // Logic to the MovePhase
        if(hole->isUsed() && hole->player()->type() == m_player->type() && !holeSelected){
            // Highlight the selected hole
            lastMove = id;
            holeSelected = true;
            hole->setState(hole->Selected);

            // Show the possibilities in green to move the selected hole
            int playableRow = id/5;
            int playableCol = id%5;
            int checkRow = 0;
            int checkCol = 0;

            for (int i=0;i<25;++i){
                checkRow = i/5;
                checkCol = i%5;
                hole = m_board [checkRow][checkCol];
                if(hole->isEmpty() || hole->isPlayable()){
                    if (abs(playableRow - checkRow) <=1 && abs(playableCol - checkCol) <=1  && i != id){
                        hole->setPlayer(nullptr);
                        hole->setState(hole->Playable);
                    }
                }
            }
        } else if(lastMove == id && holeSelected){
            // Remove the highlight of the hole and the green possibilities to move it
            lastMove = -1;
            holeSelected = false;
            hole->setState(hole->Used);
            deselectPossibility();
        } else if(hole->state() == hole->Playable && holeSelected){
            Hole* lastHole = m_board[lastMove/5][lastMove%5];
            deselectPossibility();
            lastHole->reset();
            hole->setPlayer(m_player);
            CheckWin(m_player->type());

            // End of turn after a player hole move
            holeSelected = false;
            lastMove = -1;
            emit turnEnded();

        }
    }
}
void Teeko::switchPlayer() {
    // Switch the player.
    m_player = m_player->other();

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Teeko::reset() {
    // Reset board.
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            Hole* hole = m_board[row][col];
            hole->reset();
        }
    }

    // Reset the player.
    m_player = Player::player(Player::Red);

    // Reset to drop phase.
    m_phase = Teeko::DropPhase;

    // Reset ball counts.
    m_redHolesCount = 0;
    m_blueHolesCount = 0;
    holeSelected = false;
    lastMove = -1;

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Teeko::showAbout() {
    QMessageBox::information(this, tr("About"), tr("Teeko\n\nMarcelo Soares Cassiano - marcelosoarescassiano@gmail.com"));
}

void Teeko::updateStatusBar() {
    QString phase(m_phase == Teeko::DropPhase ? tr("colocar") : tr("mover"));

    ui->statusbar->showMessage(tr("Fase de %1: vez do %2")
                               .arg(phase)
                               .arg(m_player->name()));
}

bool Teeko::isValidMove(Hole* source, Hole* target) {
    if (source == nullptr || target == nullptr) {
        return false;
    }

    int sourceRow = source->row();
    int sourceCol = source->col();
    int targetRow = target->row();
    int targetCol = target->col();

    // Check if the target hole is adjacent to the source hole (horizontal or vertical)
    if ((qAbs(sourceRow - targetRow) == 1 && sourceCol == targetCol) ||
            (qAbs(sourceCol - targetCol) == 1 && sourceRow == targetRow)) {

        // Check if the target hole is empty
        return target->isEmpty();
    }

    return false;
}

void Teeko::deselectPossibility(){
    // Deselect the moves possibilities
    Hole* holeCheck;
    for(int x=0;x<25;++x){
        holeCheck =  m_board[x/5][x%5];
        if( holeCheck->isPlayable() || holeCheck->isEmpty() ){
            holeCheck->reset();
        }
    }

}

void Teeko::CheckWin(int player){
    //Check if a player won the game
    int winConditionRow = 0;
    int winConditionCol = 0;
    int lenght = 0;

    std::vector<std::pair<int,int>> positions;

    for(int i =0;i<5;i++){
        for(int j =0;j<5;j++){
            Hole* hole =m_board[i][j];
            if(hole->isUsed() && hole->player()->type() == player){
                positions.push_back(std::make_pair(i,j));
                lenght++;
            }
        }
    }

    std::sort(positions.begin(),positions.end());

    //Check the winning condition in row or column
    for (int i =0;i<lenght;i++) {
        if(positions[i].first == (positions[i+1].first) && positions[i].second +1 == (positions[i+1].second)){
            winConditionRow++;
        }
        if(positions[i].second == (positions[i+1].second) && positions[i].first +1 == (positions[i+1].first)){
            winConditionCol++;
        }
    }
    if (winConditionCol == 3 || winConditionRow == 3){
        //Winner(player);
    } else{
        winConditionCol = 0;
        winConditionRow = 0;
    }

    //Check the winning condition in diagonal
    for (int i = 0; i<lenght-1;i++) {
        if((positions[i].first+1) == (positions[i+1].first) && (positions[i].second+1) == (positions[i+1].second)){
            winConditionRow++;
        }
        if((positions[i].second -1) == (positions[i+1].second) && (positions[i].first+1) == (positions[i+1].first)){
            winConditionCol++;
        }
    }
    if (winConditionCol == 3 || winConditionRow == 3){
        //Winner(player);
    } else{
        winConditionCol = 0;
        winConditionRow = 0;
    }

    //Check the winning condition in square
    if(positions[0].first == positions[1].first && positions[2].first == positions[3].first && positions[0].first +1 == positions[2].first){
        if(positions[0].second == positions[2].second && positions[1].second == positions[3].second && positions[0].second +1 == positions[1].second){
            winConditionRow =3;
        }
    }

    if(winConditionRow == 3 || winConditionCol == 3){
        Winner(player);
    }
}

void Teeko::Winner(int player){
    // Creates a pop-up showing the winner
    if(player == 0){
        QMessageBox::information(this, tr("Vencedor"), tr("Parabéns, o Jogador Vermelho venceu"));
    } else{
        QMessageBox::information(this, tr("Vencedor"), tr("Parabéns, o Jogador Azul venceu"));
    }
    reset();
    switchPlayer();
}

