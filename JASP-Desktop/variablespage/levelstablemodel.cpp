
#include "levelstablemodel.h"
#include <boost/foreach.hpp>
#include <boost/container/vector.hpp>
#include <algorithm>
#include <QDebug>
#include <QColor>

#include "qutils.h"

LevelsTableModel::LevelsTableModel(QObject *parent)
	: QAbstractTableModel(parent)
{
	_column = NULL;
}

void LevelsTableModel::setColumn(Column *column)
{
	beginResetModel();
	_column = column;
	endResetModel();
	emit resizeValueColumn();
}

void LevelsTableModel::refresh()
{
	beginResetModel();
	endResetModel();
	emit resizeValueColumn();
	emit labelFilterChanged();
}

void LevelsTableModel::clearColumn()
{
	setColumn(NULL);
}

int LevelsTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	if (_column == NULL)
		return 0;

	return _column->labels().size();
}

int LevelsTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return 3;
}

QVariant LevelsTableModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::BackgroundColorRole && index.column() == 0)
		return QColor(0xf6,0xf6,0xf6);

	// (role != Qt::DisplayRole && role != Qt::EditRole)
	//	return QVariant();

	Labels &labels = _column->labels();
	int row = index.row();


	if(role == (int)Roles::ValueRole) return tq(labels.getValueFromRow(row));
	if(role == (int)Roles::LabelRole) return tq(labels.getLabelFromRow(row));
	if(role == (int)Roles::FilterRole) return QVariant(labels[row].filterAllows());

	if(role == Qt::DisplayRole)
	{
		if (index.column() == 0)
			return tq(labels.getValueFromRow(row));
		else if(index.column() == 1)
			return tq(labels.getLabelFromRow(row));
		else if(index.column() == 2)
			return QVariant(labels[row].filterAllows());
	}

	return QVariant();
}

QVariant LevelsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == (int)Roles::ValueRole) return "Value";
	if(role == (int)Roles::LabelRole) return "Label";
	if(role == (int)Roles::FilterRole) return "Filter";

	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation != Qt::Horizontal)
		return QVariant();

	if (section == 0)
		return "Value";
	else
		return "Label";
}

void LevelsTableModel::_moveRows(QModelIndexList &selection, bool up) {
	if (_column == NULL)
		return;

	Labels &labels = _column->labels();
	std::vector<Label> new_labels(labels.begin(), labels.end());

	BOOST_FOREACH (QModelIndex &index, selection)
	{
		if (index.column() == 0) {
			iter_swap(new_labels.begin() + index.row(), new_labels.begin() + (index.row() + (up ? - 1: 1)));
		}
	}
	labels.set(new_labels);

	QModelIndex topLeft = createIndex(0,0);
	QModelIndex bottonRight = createIndex(labels.size() - 1, 1);
	//emit a signal to make the view reread identified data
	emit dataChanged(topLeft, bottonRight);
}

void LevelsTableModel::moveUp(QModelIndexList &selection) {
	_moveRows(selection, true);
}

void LevelsTableModel::moveDown(QModelIndexList &selection) {
	_moveRows(selection, false);
}

void LevelsTableModel::reverse() {
    if (_column == NULL)
        return;

    Labels &labels = _column->labels();
	std::vector<Label> new_labels(labels.begin(), labels.end());

    std::reverse(new_labels.begin(), new_labels.end());

    labels.set(new_labels);

    QModelIndex topLeft = createIndex(0,0);
    QModelIndex bottonRight = createIndex(labels.size() - 1, 1);
    //emit a signal to make the view reread identified data
    emit dataChanged(topLeft, bottonRight);
}

Qt::ItemFlags LevelsTableModel::flags(const QModelIndex &index) const
{
	if (index.column() == 1) {
        return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    } else {
        return QAbstractTableModel::flags(index);
	}
}

bool LevelsTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (_column == NULL)
        return false;

    if (role == Qt::EditRole)
    {
        const std::string &new_label = value.toString().toStdString();
        if (new_label != "") {
            Labels &labels = _column->labels();
			if (labels.setLabelFromRow(index.row(), new_label))
				emit dataChanged(index, index);

			emit refreshConnectedModels(_column);
			emit labelFilterChanged();
		}
    }

    return true;
}


QHash<int, QByteArray> LevelsTableModel::roleNames() const
{
	static const QHash<int, QByteArray> roles = QHash<int, QByteArray> { {(int)Roles::ValueRole, "value"}, {(int)Roles::LabelRole, "label"}, {(int)Roles::FilterRole, "filter"} };
	return roles;
}


QModelIndexList LevelsTableModel::convertQVariantList_to_QModelIndexList(QVariantList selection)
{
	QModelIndexList List;
	bool Converted;

	for(QVariant variant : selection)
	{
		int Row = variant.toInt(&Converted);
		if(Converted)
			List << index(Row, 0);
	}

	return List;

}

void LevelsTableModel::setAllowFilterOnLabel(int row, bool newAllowValue)
{
	bool atLeastOneRemains = newAllowValue;

	if(!atLeastOneRemains) //Do not let the user uncheck every single one because that is useless, the user wants to uncheck row so lets see if there is another one left after that.
		for(int i=0; i< _column->labels().size(); i++)
			if(i != row && _column->labels()[i].filterAllows())
			{
				atLeastOneRemains = true;
				break;
			}

	if(atLeastOneRemains)
	{
		_column->labels()[row].setFilterAllows(newAllowValue);
		emit labelFilterChanged();
		emit dataChanged(index(row, 2), index(row, 2)); //to make sure the checkbox is set to the right value
	}


}

bool LevelsTableModel::allowFilter(int row)
{
	return _column->labels()[row].filterAllows();
}
