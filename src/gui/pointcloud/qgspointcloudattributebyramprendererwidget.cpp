/***************************************************************************
    qgspointcloudattributebyramprendererwidget.cpp
    ---------------------
    begin                : November 2020
    copyright            : (C) 2020 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgspointcloudattributebyramprendererwidget.h"
#include "qgscontrastenhancement.h"
#include "qgspointcloudlayer.h"
#include "qgspointcloudattributebyramprenderer.h"
#include "qgsdoublevalidator.h"
#include "qgsstyle.h"

///@cond PRIVATE

QgsPointCloudAttributeByRampRendererWidget::QgsPointCloudAttributeByRampRendererWidget( QgsPointCloudLayer *layer, QgsStyle *style )
  : QgsPointCloudRendererWidget( layer, style )
{
  setupUi( this );

  mAttributeComboBox->setAllowEmptyAttributeName( false );
  mAttributeComboBox->setFilters( QgsPointCloudAttributeProxyModel::Numeric );

  if ( layer )
  {
    mAttributeComboBox->setLayer( layer );

    setFromRenderer( layer->renderer() );
  }

  connect( mAttributeComboBox, &QgsPointCloudAttributeComboBox::attributeChanged,
           this, &QgsPointCloudAttributeByRampRendererWidget::attributeChanged );
  connect( mMinSpin, qgis::overload<double>::of( &QDoubleSpinBox::valueChanged ), this, &QgsPointCloudAttributeByRampRendererWidget::minMaxChanged );
  connect( mMaxSpin, qgis::overload<double>::of( &QDoubleSpinBox::valueChanged ), this, &QgsPointCloudAttributeByRampRendererWidget::minMaxChanged );

  connect( mScalarColorRampShaderWidget, &QgsColorRampShaderWidget::widgetChanged, this, &QgsPointCloudAttributeByRampRendererWidget::emitWidgetChanged );
  connect( mScalarRecalculateMinMaxButton, &QPushButton::clicked, this, &QgsPointCloudAttributeByRampRendererWidget::setMinMaxFromLayer );

  attributeChanged();
}

QgsPointCloudRendererWidget *QgsPointCloudAttributeByRampRendererWidget::create( QgsPointCloudLayer *layer, QgsStyle *style, QgsPointCloudRenderer * )
{
  return new QgsPointCloudAttributeByRampRendererWidget( layer, style );
}

QgsPointCloudRenderer *QgsPointCloudAttributeByRampRendererWidget::renderer()
{
  if ( !mLayer )
  {
    return nullptr;
  }

  std::unique_ptr< QgsPointCloudAttributeByRampRenderer > renderer = qgis::make_unique< QgsPointCloudAttributeByRampRenderer >();
  renderer->setAttribute( mAttributeComboBox->currentAttribute() );

  renderer->setMinimum( mMinSpin->value() );
  renderer->setMaximum( mMaxSpin->value() );

  renderer->setColorRampShader( mScalarColorRampShaderWidget->shader() );

  return renderer.release();
}

void QgsPointCloudAttributeByRampRendererWidget::emitWidgetChanged()
{
  if ( !mBlockChangedSignal )
    emit widgetChanged();
}

void QgsPointCloudAttributeByRampRendererWidget::minMaxChanged()
{
  if ( mBlockMinMaxChanged )
    return;

  mScalarColorRampShaderWidget->setMinimumMaximumAndClassify( mMinSpin->value(), mMaxSpin->value() );
}

void QgsPointCloudAttributeByRampRendererWidget::attributeChanged()
{
  if ( mLayer && mLayer->dataProvider() )
  {
    const QVariant min = mLayer->dataProvider()->metadataStatistic( mAttributeComboBox->currentAttribute(), QgsStatisticalSummary::Min );
    const QVariant max = mLayer->dataProvider()->metadataStatistic( mAttributeComboBox->currentAttribute(), QgsStatisticalSummary::Max );
    if ( min.isValid() && max.isValid() )
    {
      mProviderMin = min.toDouble();
      mProviderMax = max.toDouble();
    }
    else
    {
      mProviderMin = std::numeric_limits< double >::quiet_NaN();
      mProviderMax = std::numeric_limits< double >::quiet_NaN();
    }
  }
  mScalarRecalculateMinMaxButton->setEnabled( !std::isnan( mProviderMin ) && !std::isnan( mProviderMax ) );
  emitWidgetChanged();
}

void QgsPointCloudAttributeByRampRendererWidget::setMinMaxFromLayer()
{
  if ( std::isnan( mProviderMin ) || std::isnan( mProviderMax ) )
    return;

  mBlockMinMaxChanged = true;
  mMinSpin->setValue( mProviderMin );
  mMaxSpin->setValue( mProviderMax );
  mBlockMinMaxChanged = false;

  minMaxChanged();
}

void QgsPointCloudAttributeByRampRendererWidget::setFromRenderer( const QgsPointCloudRenderer *r )
{
  mBlockChangedSignal = true;
  const QgsPointCloudAttributeByRampRenderer *mbcr = dynamic_cast<const QgsPointCloudAttributeByRampRenderer *>( r );
  if ( mbcr )
  {
    mAttributeComboBox->setAttribute( mbcr->attribute() );

    mMinSpin->setValue( mbcr->minimum() );
    mMaxSpin->setValue( mbcr->maximum() );

    whileBlocking( mScalarColorRampShaderWidget )->setFromShader( mbcr->colorRampShader() );
    whileBlocking( mScalarColorRampShaderWidget )->setMinimumMaximum( mbcr->minimum(), mbcr->maximum() );
  }
  else
  {
    if ( mAttributeComboBox->findText( QStringLiteral( "Intensity" ) ) > -1 )
    {
      mAttributeComboBox->setAttribute( QStringLiteral( "Intensity" ) );
    }
    else
    {
      mAttributeComboBox->setCurrentIndex( mAttributeComboBox->count() > 1 ? 1 : 0 );
    }
  }
  attributeChanged();
  mBlockChangedSignal = false;
}

///@endcond
